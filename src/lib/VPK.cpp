#include <vpkedit/VPK.h>

#include <algorithm>
#include <iterator>

#include <MD5.h>
#include <vpkedit/detail/CRC.h>

using namespace vpkedit;
using namespace vpkedit::detail;

namespace {

std::string removeVPKAndOrDirSuffix(const std::string& path) {
    std::string filename = path;
    if (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".vpk") {
        filename = filename.substr(0, filename.length() - 4);
    }

    // This indicates it's a dir VPK, but some people ignore this convention...
    // It should fail later if it's not a proper dir VPK
    if (filename.length() >= 4 && filename.substr(filename.length() - 4) == "_dir") {
        filename = filename.substr(0, filename.length() - 4);
    }

    return filename;
}

std::pair<std::string, std::string> splitFilenameAndParentDir(const std::string& filename) {
    auto name = filename;
    std::replace(name.begin(), name.end(), '\\', '/');

    auto lastSeparator = name.rfind('/');
    auto dir = lastSeparator != std::string::npos ? name.substr(0, lastSeparator) : "";
    name = filename.substr(lastSeparator + 1);

    return {dir, name};
}

std::string padArchiveIndex(int num) {
	static constexpr int WIDTH = 3;
    auto numStr = std::to_string(num);
    return std::string(WIDTH - std::min<std::string::size_type>(WIDTH, numStr.length()), '0') + numStr;
}

std::vector<std::byte> readFileData(const std::string& filepath, std::size_t preloadBytesOffset) {
	FileStream stream{filepath};
	if (!stream) {
		return {};
	}
	stream.seekInput(preloadBytesOffset);
	return stream.readBytes(std::filesystem::file_size(filepath) - preloadBytesOffset);
}

} // namespace

VPK::VPK(FileStream&& reader_, std::string fullPath_, std::string filename_, std::uint32_t preferredChunkSize_)
        : reader(std::move(reader_))
        , fullPath(std::move(fullPath_))
        , filename(std::move(filename_))
		, preferredChunkSize(preferredChunkSize_) {}

VPK VPK::createEmpty(const std::string& path, VPKOptions options) {
	{
        FileStream stream{path, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};

        Header1 header1{};
        header1.signature = VPK_ID;
        header1.version = options.version;
        header1.treeSize = 1;
        stream.write(&header1);

        if (options.version != 1) {
            Header2 header2{};
            header2.fileDataSectionSize = 0;
            header2.archiveMD5SectionSize = 0;
            header2.otherMD5SectionSize = 0;
            header2.signatureSectionSize = 0;
            stream.write(&header2);
        }

		stream.write('\0');
    }
    return *VPK::open(path, options.preferredChunkSize);
}

VPK VPK::createFromDirectory(const std::string& vpkPath, const std::string& contentPath, bool saveToDir, VPKOptions options, const Callback& callback) {
    auto vpk = VPK::createEmpty(vpkPath, options);
	if (!std::filesystem::exists(contentPath) || std::filesystem::status(contentPath).type() != std::filesystem::file_type::directory) {
		return vpk;
	}
    for (const auto& file : std::filesystem::recursive_directory_iterator(contentPath)) {
        if (file.is_directory()) {
            continue;
        }
	    std::string entryPath;
		try {
			entryPath = std::filesystem::absolute(file.path()).string().substr(std::filesystem::absolute(contentPath).string().length());
		} catch (const std::exception&) {
			continue;
		}
        if (entryPath.empty()) {
            continue;
        }
        while (entryPath.at(0) == '/' || entryPath.at(0) == '\\') {
            entryPath = entryPath.substr(1);
        }
        vpk.addEntry(entryPath, file.path().string(), saveToDir);
    }
    vpk.bake("", callback);
    return vpk;
}

std::optional<VPK> VPK::open(const std::string& path, std::uint32_t preferredChunkSize, const Callback& callback) {
    if (!std::filesystem::exists(path)) {
        // File does not exist
        return std::nullopt;
    }

    std::string fileNameNoSuffix = ::removeVPKAndOrDirSuffix(path);

    VPK vpk{FileStream{path}, path, fileNameNoSuffix, preferredChunkSize};
    if (VPK::open(vpk, callback)) {
        return vpk;
    }
    return std::nullopt;
}

std::optional<VPK> VPK::open(std::byte* buffer, std::uint64_t bufferLen, std::uint32_t preferredChunkSize, const Callback& callback) {
    VPK vpk{FileStream{buffer, bufferLen}, "", "", preferredChunkSize};
    if (VPK::open(vpk, callback)) {
        return vpk;
    }
    return std::nullopt;
}

bool VPK::open(VPK& vpk, const Callback& callback) {
    vpk.reader.seekInput(0);
    vpk.reader.read(vpk.header1);
    if (vpk.header1.signature != VPK_ID) {
        // File is not a VPK
        return false;
    }
    if (vpk.header1.version == 2) {
        vpk.reader.read(vpk.header2);
    } else if (vpk.header1.version != 1) {
        // Apex Legends, Titanfall, etc. are not supported
        return false;
    }

    // Extensions
    while (true) {
        std::string extension;
        vpk.reader.read(extension);
        if (extension.empty())
            break;

        // Directories
        while (true) {
            std::string directory;
            vpk.reader.read(directory);
            if (directory.empty())
                break;

            // Files
            while (true) {
                std::string entryname;
                vpk.reader.read(entryname);
                if (entryname.empty())
                    break;

                VPKEntry entry{};

                if (extension == " ") {
                    entry.filename = entryname;
                } else {
                    entry.filename = entryname + '.';
                    entry.filename += extension;
                }

                std::string fullDir;
                if (directory == " ") {
                    fullDir = "";
                } else {
                    fullDir = directory;
                }
                if (!vpk.entries.count(fullDir)) {
                    vpk.entries[fullDir] = {};
                }

                vpk.reader.read(entry.crc32);
                auto preloadedDataSize = vpk.reader.read<std::uint16_t>();
                vpk.reader.read(entry.archiveIndex);
                vpk.reader.read(entry.offset);
                vpk.reader.read(entry.length);

                if (vpk.reader.read<std::uint16_t>() != VPK_ENTRY_TERM) {
                    // Invalid terminator!
                    return false;
                }

                if (preloadedDataSize > 0) {
                    entry.preloadedData = vpk.reader.readBytes(preloadedDataSize);
                    entry.length += preloadedDataSize;
                }

                vpk.entries[fullDir].push_back(entry);

                if (entry.archiveIndex != VPK_DIR_INDEX && entry.archiveIndex > vpk.numArchives) {
                    vpk.numArchives = entry.archiveIndex;
                }

				if (callback) {
					callback(fullDir, entry);
				}
            }
        }
    }

    // If there are no archives, -1 will be incremented to 0
    vpk.numArchives++;

    // Read VPK2-specific data
    if (vpk.header1.version != 2)
        return true;

    // Skip over file data, if any
    vpk.reader.seekInput(vpk.header2.fileDataSectionSize, std::ios_base::cur);

    if (vpk.header2.archiveMD5SectionSize % sizeof(MD5Entry) != 0)
        return false;

    vpk.md5Entries.clear();
    unsigned int entryNum = vpk.header2.archiveMD5SectionSize / sizeof(MD5Entry);
    for (unsigned int i = 0; i < entryNum; i++)
        vpk.md5Entries.push_back(vpk.reader.read<MD5Entry>());

    if (vpk.header2.otherMD5SectionSize != 48)
	    // This should always be 48
        return true;

	vpk.footer2.treeChecksum = vpk.reader.readBytes<16>();
	vpk.footer2.md5EntriesChecksum = vpk.reader.readBytes<16>();
	vpk.footer2.wholeFileChecksum = vpk.reader.readBytes<16>();

    if (!vpk.header2.signatureSectionSize) {
        return true;
    }

    auto publicKeySize = vpk.reader.read<std::int32_t>();
    if (vpk.header2.signatureSectionSize == 20 && publicKeySize == VPK_ID) {
        // CS2 beta VPK, ignore it
        return true;
    }

    vpk.footer2.publicKey = vpk.reader.readBytes(publicKeySize);
    vpk.footer2.signature = vpk.reader.readBytes(vpk.reader.read<std::int32_t>());

    return true;
}

std::optional<VPKEntry> VPK::findEntry(const std::string& filename_, bool includeUnbaked) const {
    auto [dir, name] = ::splitFilenameAndParentDir(filename_);
    if (!dir.empty()) {
        if (dir.length() > 1 && dir.substr(0, 1) == "/") {
            dir = dir.substr(1);
        }
        if (dir.length() > 2 && dir.substr(dir.length() - 1) == "/") {
            dir = dir.substr(0, dir.length() - 2);
        }
    }
    if (this->entries.count(dir)) {
        for (const VPKEntry& entry : this->entries.at(dir)) {
            if (entry.filename == name) {
                return entry;
            }
        }
    }
    if (includeUnbaked && this->unbakedEntries.count(dir)) {
        for (const VPKEntry& unbakedEntry : this->unbakedEntries.at(dir)) {
            if (unbakedEntry.filename == name) {
                return unbakedEntry;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::vector<std::byte>> VPK::readBinaryEntry(const VPKEntry& entry) const {
    std::vector output(entry.length, static_cast<std::byte>(0));

    if (!entry.preloadedData.empty()) {
        std::copy(entry.preloadedData.begin(), entry.preloadedData.end(), output.begin());
    }

    if (entry.length == entry.preloadedData.size()) {
        return output;
    }

    if (entry.unbaked) {
        // Get the stored data
        for (const auto& [unbakedEntryDir, unbakedEntryList] : this->unbakedEntries) {
            for (const VPKEntry& unbakedEntry : unbakedEntryList) {
                if (unbakedEntry.filename == entry.filename) {
	                std::vector<std::byte> unbakedData;
	                if (unbakedEntry.unbakedUsingByteBuffer) {
		                unbakedData = std::get<std::vector<std::byte>>(unbakedEntry.unbakedData);
	                } else {
	                    unbakedData = ::readFileData(std::get<std::string>(unbakedEntry.unbakedData), unbakedEntry.preloadedData.size());
                    }
	                std::copy(unbakedData.begin(), unbakedData.end(), output.begin() + static_cast<long long>(entry.preloadedData.size()));
	                return output;
                }
            }
        }
    } else if (entry.archiveIndex != VPK_DIR_INDEX) {
        FileStream stream{this->filename + '_' + ::padArchiveIndex(entry.archiveIndex) + ".vpk"};
        if (!stream) {
            return std::nullopt;
        }
        stream.seekInput(entry.offset);
        auto bytes = stream.readBytes(entry.length - entry.preloadedData.size());
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.preloadedData.size()));
    } else if (!filename.empty()) {
        FileStream stream{this->fullPath};
        if (!stream) {
            return std::nullopt;
        }
        stream.seekInput(this->getHeaderLength() + this->header1.treeSize + entry.offset);
        auto bytes = stream.readBytes(entry.length - entry.preloadedData.size());
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.preloadedData.size()));
    } else {
        // Loaded from memory, but file is not in the directory VPK!
        return std::nullopt;
    }

    return output;
}

std::optional<std::string> VPK::readTextEntry(const VPKEntry& entry) const {
    auto bytes = this->readBinaryEntry(entry);
    if (!bytes) {
        return std::nullopt;
    }
    std::string out;
    for (auto byte : *bytes) {
        if (byte == static_cast<std::byte>(0))
            break;
        out += static_cast<char>(byte);
    }
    return out;
}

void VPK::addEntry(const std::string& filename_, const std::string& pathToFile, bool saveToDir, int preloadBytes) {
	const auto [dir, name] = ::splitFilenameAndParentDir(filename_);
	// We process preload bytes later
	auto buffer = ::readFileData(pathToFile, 0);

	VPKEntry entry{};
	entry.unbaked = true;
	entry.unbakedUsingByteBuffer = false;
	entry.filename = name;

	entry.crc32 = computeCRC(buffer);
	entry.length = buffer.size();

	// Offset and archive index might be reset when the VPK is baked
	entry.offset = 0;
	entry.archiveIndex = saveToDir ? VPK_DIR_INDEX : this->numArchives;

	if (preloadBytes > 0) {
		// Maximum preloaded data size is 1kb
		auto clampedPreloadBytes = std::clamp(preloadBytes, 0, buffer.size() > VPK_MAX_PRELOAD_BYTES ? VPK_MAX_PRELOAD_BYTES : static_cast<int>(buffer.size()));
		entry.preloadedData.resize(clampedPreloadBytes);
		std::memcpy(entry.preloadedData.data(), buffer.data(), clampedPreloadBytes);
		buffer.erase(buffer.begin(), buffer.begin() + clampedPreloadBytes);
	}

	// Now that archive index is calculated for this entry, check if it needs to be incremented
	if (!saveToDir) {
		entry.offset = this->currentlyFilledChunkSize;
		this->currentlyFilledChunkSize += static_cast<int>(buffer.size());
		if (this->preferredChunkSize) {
			if (this->currentlyFilledChunkSize > this->preferredChunkSize) {
				this->currentlyFilledChunkSize = 0;
				this->numArchives++;
			}
		}
	}

	if (!this->unbakedEntries.count(dir)) {
		this->unbakedEntries[dir] = {};
	}

	entry.unbakedData = pathToFile;
	this->unbakedEntries.at(dir).push_back(std::move(entry));
}

void VPK::addBinaryEntry(const std::string& filename_, std::vector<std::byte>&& buffer, bool saveToDir, int preloadBytes) {
    const auto [dir, name] = ::splitFilenameAndParentDir(filename_);

    VPKEntry entry{};
    entry.unbaked = true;
	entry.unbakedUsingByteBuffer = true;
    entry.filename = name;

    entry.crc32 = computeCRC(buffer);
    entry.length = buffer.size();

    // Offset and archive index might be reset when the VPK is baked
    entry.offset = 0;
    entry.archiveIndex = saveToDir ? VPK_DIR_INDEX : this->numArchives;

    if (preloadBytes > 0) {
        // Maximum preloaded data size is 1kb
        auto clampedPreloadBytes = std::clamp(preloadBytes, 0, buffer.size() > VPK_MAX_PRELOAD_BYTES ? VPK_MAX_PRELOAD_BYTES : static_cast<int>(buffer.size()));
        entry.preloadedData.resize(clampedPreloadBytes);
        std::memcpy(entry.preloadedData.data(), buffer.data(), clampedPreloadBytes);
        buffer.erase(buffer.begin(), buffer.begin() + clampedPreloadBytes);
    }

	// Now that archive index is calculated for this entry, check if it needs to be incremented
	if (!saveToDir) {
		entry.offset = this->currentlyFilledChunkSize;
		this->currentlyFilledChunkSize += static_cast<int>(buffer.size());
		if (this->preferredChunkSize) {
			if (this->currentlyFilledChunkSize > this->preferredChunkSize) {
				this->currentlyFilledChunkSize = 0;
				this->numArchives++;
			}
		}
	}

    if (!this->unbakedEntries.count(dir)) {
        this->unbakedEntries[dir] = {};
    }

    entry.unbakedData = std::move(buffer);
    this->unbakedEntries.at(dir).push_back(std::move(entry));
}

void VPK::addBinaryEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, bool saveToDir, int preloadBytes) {
	std::vector<std::byte> data;
	data.resize(bufferLen);
	std::memcpy(data.data(), buffer, bufferLen);
	this->addBinaryEntry(filename_, std::move(data), saveToDir, preloadBytes);
}

void VPK::addTextEntry(const std::string& filename_, const std::string& text, bool saveToDir, int preloadBytes) {
	std::vector<std::byte> data;
	data.reserve(text.size());
	std::transform(text.begin(), text.end(), std::back_inserter(data), [](char c) {
		return static_cast<std::byte>(c);
	});
	this->addBinaryEntry(filename_, std::move(data), saveToDir, preloadBytes);
}

bool VPK::removeEntry(const std::string& filename_) {
    const auto [dir, name] = ::splitFilenameAndParentDir(filename_);

    // Check unbaked entries first
    if (this->unbakedEntries.count(dir)) {
        for (auto& [preexistingDir, unbakedEntryVec] : this->unbakedEntries) {
            if (preexistingDir != dir) {
                continue;
            }
            for (auto it = unbakedEntryVec.begin(); it != unbakedEntryVec.end(); ++it) {
                if (it->filename == name) {
                    unbakedEntryVec.erase(it);
                    return true;
                }
            }
        }
    }

    // If it's not in regular entries either you can't remove it!
    if (!this->entries.count(dir))
        return false;

    for (auto it = this->entries.at(dir).begin(); it != this->entries.at(dir).end(); ++it) {
        if (it->filename == name) {
            this->entries.at(dir).erase(it);
            return true;
        }
    }
    return false;
}

bool VPK::bake(const std::string& outputFolder_, const Callback& callback) {
    // Loaded from memory
    if (this->fullPath.empty())
        return false;

    // Reconstruct data so we're not looping over it a ton of times
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<VPKEntry*>>> temp;

    for (auto& [tDir, tEntries] : this->entries) {
        for (auto& tEntry : tEntries) {
	        std::string extension = tEntry.getExtension();
	        if (extension.empty()) {
		        extension = " ";
	        }
            if (!temp.count(extension)) {
                temp[extension] = {};
            }
            if (!temp.at(extension).count(tDir)) {
                temp.at(extension)[tDir] = {};
            }
            temp.at(extension).at(tDir).push_back(&tEntry);
        }
    }
    for (auto& [tDir, tEntries]: this->unbakedEntries) {
        for (auto& tEntry : tEntries) {
			std::string extension = tEntry.getExtension();
			if (extension.empty()) {
				extension = " ";
			}
            if (!temp.count(extension)) {
                temp[extension] = {};
            }
            if (!temp.at(extension).count(tDir)) {
                temp.at(extension)[tDir] = {};
            }
            temp.at(extension).at(tDir).push_back(&tEntry);
        }
    }

    // Temporarily store baked file data that's stored in the directory VPK since it's getting overwritten
    std::vector<std::byte> dirVPKEntryData;
    std::size_t newDirEntryOffset = 0;
    for (auto& [tDir, tEntries] : this->entries) {
        for (auto& tEntry : tEntries) {
            if (!tEntry.unbaked && tEntry.archiveIndex == VPK_DIR_INDEX && tEntry.length != tEntry.preloadedData.size()) {
                auto binData = VPK::readBinaryEntry(tEntry);
                if (!binData) {
                    continue;
                }
                dirVPKEntryData.reserve(dirVPKEntryData.size() + tEntry.length - tEntry.preloadedData.size());
                dirVPKEntryData.insert(dirVPKEntryData.end(), binData->begin() + static_cast<std::vector<std::byte>::difference_type>(tEntry.preloadedData.size()), binData->end());

                tEntry.offset = newDirEntryOffset;
                newDirEntryOffset += tEntry.length - tEntry.preloadedData.size();
            }
        }
    }

    // Get the file output paths
    std::string outputFilename = std::filesystem::path(this->fullPath).filename().string();
    std::string outputFilenameNoExtension = this->getRealFilename();
    std::string outputFolder;
    if (!outputFolder_.empty()) {
        outputFolder = outputFolder_;
        if (outputFolder.at(outputFolder.length() - 1) == '/' || outputFolder.at(outputFolder.length() - 1) == '\\') {
            outputFolder.pop_back();
        }
        this->fullPath = outputFolder + '/' + outputFilename;
    } else {
        outputFolder = this->fullPath;
        std::replace(outputFolder.begin(), outputFolder.end(), '\\', '/');
        auto lastSlash = outputFolder.rfind('/');
        if (lastSlash != std::string::npos) {
            outputFolder = filename.substr(0, lastSlash);
        } else {
			outputFolder = "./";
		}
    }

	// Helper
	const auto getArchiveFilename = [](const std::string& filename_, int archiveIndex) {
		return filename_ + '_' + ::padArchiveIndex(archiveIndex) + ".vpk";
	};

    // Copy external binary blobs to the new dir
    auto dirVPKFilePath = outputFolder + '/' + outputFilename;
    if (!outputFolder_.empty()) {
        for (int archiveIndex = 0; archiveIndex < this->numArchives; archiveIndex++) {
	        std::string dest = outputFolder + '/';
			dest += outputFilenameNoExtension;
			dest = getArchiveFilename(dest, archiveIndex);
			if (!std::filesystem::exists(dest)) {
				continue;
			}
			std::filesystem::copy(getArchiveFilename(this->filename, archiveIndex), dest, std::filesystem::copy_options::overwrite_existing);
        }
    }

    FileStream outDir{dirVPKFilePath, FILESTREAM_OPT_READ | FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};

    // Dummy header
    outDir.seekInput(0);
    outDir.seekOutput(0);
    outDir.write(&this->header1);
    if (this->header1.version == 2) {
        outDir.write(&this->header2);
    }

    // File tree data
    for (auto& [ext, tDirs] : temp) {
        outDir.write(ext);
        outDir.write('\0');

        for (auto& [dir, tEntries] : tDirs) {
            outDir.write(!dir.empty() ? dir : " ");
            outDir.write('\0');

            for (auto* entry : tEntries) {
                // Calculate entry offset if it's unbaked and upload the data
                if (entry->unbaked) {
                    std::vector<std::byte> entryData;
					if (entry->unbakedUsingByteBuffer) {
						entryData = std::get<std::vector<std::byte>>(entry->unbakedData);
					} else {
						entryData = ::readFileData(std::get<std::string>(entry->unbakedData), entry->preloadedData.size());
					}

                    if (entry->length == entry->preloadedData.size()) {
                        // Override the archive index, no need for an archive VPK
                        entry->archiveIndex = VPK_DIR_INDEX;
                        entry->offset = dirVPKEntryData.size();
                    } else if (entry->archiveIndex != VPK_DIR_INDEX) {
						auto archiveFilename = getArchiveFilename(::removeVPKAndOrDirSuffix(dirVPKFilePath), entry->archiveIndex);
						entry->offset = std::filesystem::exists(archiveFilename) ? std::filesystem::file_size(archiveFilename) : 0;

                        FileStream stream{archiveFilename, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_APPEND | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
                        stream.write(entryData.data(), entryData.size());
                    } else {
                        entry->offset = dirVPKEntryData.size();
                        dirVPKEntryData.insert(dirVPKEntryData.end(), entryData.data(), entryData.data() + entryData.size());
                    }
                }

                outDir.write(entry->getStem());
                outDir.write('\0');
                outDir.write(entry->crc32);
                outDir.write(static_cast<std::uint16_t>(entry->preloadedData.size()));
                outDir.write(entry->archiveIndex);
                outDir.write(entry->offset);
                outDir.write(entry->length - static_cast<std::uint32_t>(entry->preloadedData.size()));
                outDir.write(VPK_ENTRY_TERM);

                if (!entry->preloadedData.empty()) {
                    outDir.writeBytes(entry->preloadedData);
                }

				if (callback) {
					callback(dir, *entry);
				}
            }
            outDir.write('\0');
        }
        outDir.write('\0');
    }
    outDir.write('\0');

    // Put files copied from the dir archive back
    if (!dirVPKEntryData.empty()) {
        outDir.writeBytes(dirVPKEntryData);
    }

    // Merge unbaked into baked entries
    for (auto& [tDir, tUnbakedEntriesAndData] : this->unbakedEntries) {
        for (VPKEntry& tUnbakedEntry : tUnbakedEntriesAndData) {
            if (!this->entries.count(tDir)) {
                this->entries[tDir] = {};
            }
            tUnbakedEntry.unbaked = false;
            this->entries.at(tDir).push_back(tUnbakedEntry);
        }
    }
    this->unbakedEntries.clear();

    // Calculate Header1
    this->header1.treeSize = outDir.tellOutput() - dirVPKEntryData.size() - this->getHeaderLength();

    // VPK v2 stuff
    if (this->header1.version != 1) {
        // Calculate hashes for all entries
        this->md5Entries.clear();
        for (const auto& [tDir, tEntries] : this->entries) {
            for (const auto& tEntry : tEntries) {
                // Believe it or not this should be safe to call by now
                auto binData = this->readBinaryEntry(tEntry);
                if (!binData) {
                    continue;
                }
                MD5Entry md5Entry{};
                md5Entry.archiveIndex = tEntry.archiveIndex;
                md5Entry.length = tEntry.length - tEntry.preloadedData.size();
                md5Entry.offset = tEntry.offset;
                md5Entry.checksum = md5(*binData);
                this->md5Entries.push_back(md5Entry);
            }
        }

        // Calculate Header2
        this->header2.fileDataSectionSize = dirVPKEntryData.size();
        this->header2.archiveMD5SectionSize = this->md5Entries.size() * sizeof(MD5Entry);
        this->header2.otherMD5SectionSize = 48;
        this->header2.signatureSectionSize = 0;

        // Calculate Footer2
        MD5 wholeFileChecksumMD5;
        {
            // Only the tree is updated in the file right now
            wholeFileChecksumMD5.update(reinterpret_cast<const std::byte*>(&this->header1), sizeof(Header1));
            wholeFileChecksumMD5.update(reinterpret_cast<const std::byte*>(&this->header2), sizeof(Header2));
        }
        {
            outDir.seekInput(sizeof(Header1) + sizeof(Header2));
            std::vector<std::byte> treeData = outDir.readBytes(this->header1.treeSize);
            wholeFileChecksumMD5.update(treeData.data(), treeData.size());
            this->footer2.treeChecksum = md5(treeData);
        }
        if (!dirVPKEntryData.empty()) {
            wholeFileChecksumMD5.update(dirVPKEntryData.data(), dirVPKEntryData.size());
        }
        {
            wholeFileChecksumMD5.update(reinterpret_cast<const std::byte*>(this->md5Entries.data()), this->md5Entries.size() * sizeof(MD5Entry));
            MD5 md5EntriesChecksumMD5;
            md5EntriesChecksumMD5.update(reinterpret_cast<const std::byte*>(this->md5Entries.data()), this->md5Entries.size() * sizeof(MD5Entry));
            md5EntriesChecksumMD5.finalize(reinterpret_cast<unsigned char*>(this->footer2.md5EntriesChecksum.data()));
        }
        wholeFileChecksumMD5.finalize(reinterpret_cast<unsigned char*>(this->footer2.wholeFileChecksum.data()));

        // We can't recalculate the signature without the private key
        this->footer2.publicKey.clear();
        this->footer2.signature.clear();
    }

    // Write new headers
    outDir.seekOutput(0);
    outDir.write(&this->header1);

    // v2 adds the MD5 hashes and file signature
    if (this->header1.version < 2) {
        return true;
    }

    outDir.write(&this->header2);

    // Add MD5 hashes
    outDir.seekOutput(sizeof(Header1) + sizeof(Header2) + this->header1.treeSize + dirVPKEntryData.size());
    outDir.write(this->md5Entries.data(), this->md5Entries.size());
    outDir.writeBytes(this->footer2.treeChecksum);
    outDir.writeBytes(this->footer2.md5EntriesChecksum);
    outDir.writeBytes(this->footer2.wholeFileChecksum);

    // The signature section is not present
    return true;
}

std::uint32_t VPK::getVersion() const {
    return this->header1.version;
}

void VPK::setVersion(std::uint32_t version) {
    if (version == this->header1.version) {
        return;
    }
    this->header1.version = version;

    // Clearing these isn't necessary, but might as well
    this->header2 = Header2{};
    this->footer2 = Footer2{};
    this->md5Entries.clear();
}

std::string VPK::getRealFilename() const {
    return std::filesystem::path{this->fullPath}.stem().string();
}
