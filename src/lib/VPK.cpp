#include <vpkedit/VPK.h>

#include <filesystem>

#include <MD5.h>
#include <vpkedit/detail/CRC.h>
#include <vpkedit/detail/Misc.h>

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

std::string padArchiveIndex(int num) {
	static constexpr int WIDTH = 3;
    auto numStr = std::to_string(num);
    return std::string(WIDTH - std::min<std::string::size_type>(WIDTH, numStr.length()), '0') + numStr;
}

} // namespace

VPK::VPK(const std::string& fullFilePath_, PackFileOptions options_)
        : PackFile(fullFilePath_, options_)
		, reader(fullFilePath_) {
	this->type = PackFileType::VPK;
}

std::unique_ptr<PackFile> VPK::createEmpty(const std::string& path, PackFileOptions options) {
	{
        FileStream stream{path, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};

        Header1 header1{};
        header1.signature = VPK_ID;
        header1.version = options.vpk_version;
        header1.treeSize = 1;
        stream.write(&header1);

        if (options.vpk_version != 1) {
            Header2 header2{};
            header2.fileDataSectionSize = 0;
            header2.archiveMD5SectionSize = 0;
            header2.otherMD5SectionSize = 0;
            header2.signatureSectionSize = 0;
            stream.write(&header2);
        }

		stream.write('\0');
    }
    return VPK::open(path, options);
}

std::unique_ptr<PackFile> VPK::createFromDirectory(const std::string& vpkPath, const std::string& contentPath, bool saveToDir, PackFileOptions options, const Callback& bakeCallback) {
    return VPK::createFromDirectoryProcedural(vpkPath, contentPath, [saveToDir](const std::string&) {
		return std::make_tuple(saveToDir, 0);
	}, options, bakeCallback);
}

std::unique_ptr<PackFile> VPK::createFromDirectoryProcedural(const std::string& vpkPath, const std::string& contentPath, const EntryCreationCallback& creationCallback, PackFileOptions options, const Callback& bakeCallback) {
	auto vpk = VPK::createEmpty(vpkPath, options);
	if (!std::filesystem::exists(contentPath) || std::filesystem::status(contentPath).type() != std::filesystem::file_type::directory) {
		return vpk;
	}
	for (const auto& file : std::filesystem::recursive_directory_iterator(contentPath, std::filesystem::directory_options::skip_permission_denied)) {
		if (!file.is_regular_file()) {
			continue;
		}
		std::string entryPath;
		try {
			entryPath = std::filesystem::absolute(file.path()).string().substr(std::filesystem::absolute(contentPath).string().length());
			::normalizeSlashes(entryPath);
		} catch (const std::exception&) {
			continue; // Likely a Unicode error, unsupported filename
		}
		if (entryPath.empty()) {
			continue;
		}
		while (entryPath.at(0) == '/') {
			entryPath = entryPath.substr(1);
		}
		if (creationCallback) {
			auto [saveToDir, preloadBytes] = creationCallback(entryPath);
			vpk->addEntry(entryPath, file.path().string(), { .vpk_saveToDirectory = saveToDir, .vpk_preloadBytes = preloadBytes });
		} else {
			vpk->addEntry(entryPath, file.path().string(), {});
		}
	}
	vpk->bake("", bakeCallback);
	return vpk;
}

std::unique_ptr<PackFile> VPK::open(const std::string& path, PackFileOptions options, const Callback& callback) {
    if (!std::filesystem::exists(path)) {
        // File does not exist
        return nullptr;
    }

	auto* vpk = new VPK{path, options};
    auto packFile = std::unique_ptr<PackFile>(vpk);

    vpk->reader.seek_input(0);
    vpk->reader.read(vpk->header1);
    if (vpk->header1.signature != VPK_ID) {
        // File is not a VPK
        return nullptr;
    }
	// Might as well
	vpk->options.vpk_version = vpk->header1.version;
    if (vpk->header1.version == 2) {
        vpk->reader.read(vpk->header2);
    } else if (vpk->header1.version != 1) {
        // Apex Legends, Titanfall, etc. are not supported
        return nullptr;
    }

    // Extensions
    while (true) {
        std::string extension;
        vpk->reader.read(extension);
        if (extension.empty())
            break;

        // Directories
        while (true) {
            std::string directory;
            vpk->reader.read(directory);
            if (directory.empty())
                break;

	        std::string fullDir;
	        if (directory == " ") {
		        fullDir = "";
	        } else {
		        fullDir = directory;
	        }
	        if (!vpk->entries.contains(fullDir)) {
		        vpk->entries[fullDir] = {};
	        }

            // Files
            while (true) {
                std::string entryName;
                vpk->reader.read(entryName);
                if (entryName.empty())
                    break;

                Entry entry = createNewEntry();

                if (extension == " ") {
					entry.path = fullDir.empty() ? "" : fullDir + '/';
					entry.path += entryName;
                } else {
	                entry.path = fullDir.empty() ? "" : fullDir + '/';
					entry.path += entryName + '.';
                    entry.path += extension;
                }

                vpk->reader.read(entry.crc32);
                auto preloadedDataSize = vpk->reader.read<std::uint16_t>();
                vpk->reader.read(entry.vpk_archiveIndex);
                vpk->reader.read(entry.vpk_offset);
                vpk->reader.read(entry.length);

                if (vpk->reader.read<std::uint16_t>() != VPK_ENTRY_TERM) {
                    // Invalid terminator!
                    return nullptr;
                }

                if (preloadedDataSize > 0) {
                    entry.vpk_preloadedData = vpk->reader.read_bytes(preloadedDataSize);
                    entry.length += preloadedDataSize;
                }

                vpk->entries[fullDir].push_back(entry);

                if (entry.vpk_archiveIndex != VPK_DIR_INDEX && entry.vpk_archiveIndex > vpk->numArchives) {
                    vpk->numArchives = entry.vpk_archiveIndex;
                }

				if (callback) {
					callback(fullDir, entry);
				}
            }
        }
    }

    // If there are no archives, -1 will be incremented to 0
    vpk->numArchives++;

    // Read VPK2-specific data
    if (vpk->header1.version != 2)
        return packFile;

    // Skip over file data, if any
    vpk->reader.seek_input(vpk->header2.fileDataSectionSize, std::ios_base::cur);

    if (vpk->header2.archiveMD5SectionSize % sizeof(MD5Entry) != 0)
        return nullptr;

    vpk->md5Entries.clear();
    unsigned int entryNum = vpk->header2.archiveMD5SectionSize / sizeof(MD5Entry);
    for (unsigned int i = 0; i < entryNum; i++)
        vpk->md5Entries.push_back(vpk->reader.read<MD5Entry>());

    if (vpk->header2.otherMD5SectionSize != 48)
	    // This should always be 48
        return packFile;

	vpk->footer2.treeChecksum = vpk->reader.read_bytes<16>();
	vpk->footer2.md5EntriesChecksum = vpk->reader.read_bytes<16>();
	vpk->footer2.wholeFileChecksum = vpk->reader.read_bytes<16>();

    if (!vpk->header2.signatureSectionSize) {
        return packFile;
    }

    auto publicKeySize = vpk->reader.read<std::int32_t>();
    if (vpk->header2.signatureSectionSize == 20 && publicKeySize == VPK_ID) {
        // CS2 beta VPK, ignore it
        return packFile;
    }

    vpk->footer2.publicKey = vpk->reader.read_bytes(publicKeySize);
    vpk->footer2.signature = vpk->reader.read_bytes(vpk->reader.read<std::int32_t>());

    return packFile;
}

std::optional<std::vector<std::byte>> VPK::readEntry(const Entry& entry) const {
    std::vector output(entry.length, static_cast<std::byte>(0));

    if (!entry.vpk_preloadedData.empty()) {
        std::copy(entry.vpk_preloadedData.begin(), entry.vpk_preloadedData.end(), output.begin());
    }

    if (entry.length == entry.vpk_preloadedData.size()) {
        return output;
    }

    if (entry.unbaked) {
        // Get the stored data
        for (const auto& [unbakedEntryDir, unbakedEntryList] : this->unbakedEntries) {
            for (const Entry& unbakedEntry : unbakedEntryList) {
                if (unbakedEntry.path == entry.path) {
	                std::vector<std::byte> unbakedData;
	                if (isEntryUnbakedUsingByteBuffer(unbakedEntry)) {
		                unbakedData = std::get<std::vector<std::byte>>(getEntryUnbakedData(unbakedEntry));
	                } else {
	                    unbakedData = ::readFileData(std::get<std::string>(getEntryUnbakedData(unbakedEntry)), unbakedEntry.vpk_preloadedData.size());
                    }
	                std::copy(unbakedData.begin(), unbakedData.end(), output.begin() + static_cast<long long>(entry.vpk_preloadedData.size()));
	                return output;
                }
            }
        }
		return std::nullopt;
    } else if (entry.vpk_archiveIndex != VPK_DIR_INDEX) {
		// Stored in a numbered archive
        FileStream stream{this->getTruncatedFilepath() + '_' + ::padArchiveIndex(entry.vpk_archiveIndex) + ".vpk"};
        if (!stream) {
            return std::nullopt;
        }
        stream.seek_input(entry.vpk_offset);
        auto bytes = stream.read_bytes(entry.length - entry.vpk_preloadedData.size());
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.vpk_preloadedData.size()));
    } else {
		// Stored in this directory VPK
        FileStream stream{this->fullFilePath};
        if (!stream) {
            return std::nullopt;
        }
        stream.seek_input(this->getHeaderLength() + this->header1.treeSize + entry.vpk_offset);
        auto bytes = stream.read_bytes(entry.length - entry.vpk_preloadedData.size());
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.vpk_preloadedData.size()));
    }

    return output;
}

Entry& VPK::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	auto filename = filename_;
	if (!this->options.allowUppercaseLettersInFilenames) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

	entry.path = filename;
	entry.crc32 = computeCRC(buffer);
	entry.length = buffer.size();

	// Offset and archive index might be reset when the VPK is baked
	entry.vpk_offset = 0;
	entry.vpk_archiveIndex = options_.vpk_saveToDirectory ? VPK_DIR_INDEX : this->numArchives;

	if (options_.vpk_preloadBytes > 0) {
		auto clampedPreloadBytes = std::clamp(options_.vpk_preloadBytes, 0u, buffer.size() > VPK_MAX_PRELOAD_BYTES ? VPK_MAX_PRELOAD_BYTES : static_cast<std::uint32_t>(buffer.size()));
		entry.vpk_preloadedData.resize(clampedPreloadBytes);
		std::memcpy(entry.vpk_preloadedData.data(), buffer.data(), clampedPreloadBytes);
		buffer.erase(buffer.begin(), buffer.begin() + clampedPreloadBytes);
	}

	// Now that archive index is calculated for this entry, check if it needs to be incremented
	if (!options_.vpk_saveToDirectory) {
		entry.vpk_offset = this->currentlyFilledChunkSize;
		this->currentlyFilledChunkSize += static_cast<int>(buffer.size());
		if (this->options.vpk_preferredChunkSize) {
			if (this->currentlyFilledChunkSize > this->options.vpk_preferredChunkSize) {
				this->currentlyFilledChunkSize = 0;
				this->numArchives++;
			}
		}
	}

	if (!this->unbakedEntries.contains(dir)) {
		this->unbakedEntries[dir] = {};
	}
	this->unbakedEntries.at(dir).push_back(entry);
	return this->unbakedEntries.at(dir).back();
}

bool VPK::bake(const std::string& outputFolder_, const Callback& callback) {
    // Reconstruct data so we're not looping over it a ton of times
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<Entry*>>> temp;

    for (auto& [tDir, tEntries] : this->entries) {
        for (auto& tEntry : tEntries) {
	        std::string extension = tEntry.getExtension();
	        if (extension.empty()) {
		        extension = " ";
	        }
            if (!temp.contains(extension)) {
                temp[extension] = {};
            }
            if (!temp.at(extension).contains(tDir)) {
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
            if (!temp.contains(extension)) {
                temp[extension] = {};
            }
            if (!temp.at(extension).contains(tDir)) {
                temp.at(extension)[tDir] = {};
            }
            temp.at(extension).at(tDir).push_back(&tEntry);
        }
    }

    // Temporarily store baked file data that's stored in the directory VPK since it's getting overwritten
    std::vector<std::byte> dirVPKEntryData;
    std::size_t newDirEntryOffset = 0;
    for (auto& [tDir, tEntries] : this->entries) {
        for (Entry& tEntry : tEntries) {
            if (!tEntry.unbaked && tEntry.vpk_archiveIndex == VPK_DIR_INDEX && tEntry.length != tEntry.vpk_preloadedData.size()) {
                auto binData = this->readEntry(tEntry);
                if (!binData) {
                    continue;
                }
                dirVPKEntryData.reserve(dirVPKEntryData.size() + tEntry.length - tEntry.vpk_preloadedData.size());
                dirVPKEntryData.insert(dirVPKEntryData.end(), binData->begin() + static_cast<std::vector<std::byte>::difference_type>(tEntry.vpk_preloadedData.size()), binData->end());

                tEntry.vpk_offset = newDirEntryOffset;
                newDirEntryOffset += tEntry.length - tEntry.vpk_preloadedData.size();
            }
        }
    }

    // Get the file output paths
    std::string outputFilename = std::filesystem::path(this->fullFilePath).filename().string();
    std::string outputFolder;
    if (!outputFolder_.empty()) {
        outputFolder = outputFolder_;
        if (outputFolder.at(outputFolder.length() - 1) == '/' || outputFolder.at(outputFolder.length() - 1) == '\\') {
            outputFolder.pop_back();
        }
        this->fullFilePath = outputFolder + '/' + outputFilename;
    } else {
        outputFolder = this->fullFilePath;
        ::normalizeSlashes(outputFolder);
        auto lastSlash = outputFolder.rfind('/');
        if (lastSlash != std::string::npos) {
            outputFolder = this->getTruncatedFilepath().substr(0, lastSlash);
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
			auto from = getArchiveFilename(this->getTruncatedFilepath(), archiveIndex);
	        if (!std::filesystem::exists(from)) {
		        continue;
	        }
	        std::string dest = getArchiveFilename(outputFolder + '/' + this->getTruncatedFilestem(), archiveIndex);
			if (from == dest) {
				continue;
			}
			std::filesystem::copy(from, dest, std::filesystem::copy_options::overwrite_existing);
        }
    }

    FileStream outDir{dirVPKFilePath, FILESTREAM_OPT_READ | FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};

    // Dummy header
    outDir.seek_input(0);
    outDir.seek_output(0);
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
					if (isEntryUnbakedUsingByteBuffer(*entry)) {
						entryData = std::get<std::vector<std::byte>>(getEntryUnbakedData(*entry));
					} else {
						entryData = ::readFileData(std::get<std::string>(getEntryUnbakedData(*entry)), entry->vpk_preloadedData.size());
					}

                    if (entry->length == entry->vpk_preloadedData.size()) {
                        // Override the archive index, no need for an archive VPK
                        entry->vpk_archiveIndex = VPK_DIR_INDEX;
                        entry->vpk_offset = dirVPKEntryData.size();
                    } else if (entry->vpk_archiveIndex != VPK_DIR_INDEX) {
						auto archiveFilename = getArchiveFilename(::removeVPKAndOrDirSuffix(dirVPKFilePath), entry->vpk_archiveIndex);
						entry->vpk_offset = std::filesystem::exists(archiveFilename) ? std::filesystem::file_size(archiveFilename) : 0;

                        FileStream stream{archiveFilename, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_APPEND | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
                        stream.write_unsafe(entryData.data(), entryData.size());
                    } else {
                        entry->vpk_offset = dirVPKEntryData.size();
                        dirVPKEntryData.insert(dirVPKEntryData.end(), entryData.data(), entryData.data() + entryData.size());
                    }
                }

                outDir.write(entry->getStem());
                outDir.write('\0');
                outDir.write(entry->crc32);
                outDir.write(static_cast<std::uint16_t>(entry->vpk_preloadedData.size()));
                outDir.write(entry->vpk_archiveIndex);
                outDir.write(entry->vpk_offset);
                outDir.write(entry->length - static_cast<std::uint32_t>(entry->vpk_preloadedData.size()));
                outDir.write(VPK_ENTRY_TERM);

                if (!entry->vpk_preloadedData.empty()) {
                    outDir.write_bytes(entry->vpk_preloadedData);
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
        outDir.write_bytes(dirVPKEntryData);
    }

    // Merge unbaked into baked entries
    for (auto& [tDir, tUnbakedEntriesAndData] : this->unbakedEntries) {
        for (Entry& tUnbakedEntry : tUnbakedEntriesAndData) {
            if (!this->entries.contains(tDir)) {
                this->entries[tDir] = {};
            }
            tUnbakedEntry.unbaked = false;
            this->entries.at(tDir).push_back(tUnbakedEntry);
        }
    }
    this->unbakedEntries.clear();

    // Calculate Header1
    this->header1.treeSize = outDir.tell_output() - dirVPKEntryData.size() - this->getHeaderLength();

    // VPK v2 stuff
    if (this->header1.version != 1) {
        // Calculate hashes for all entries
        this->md5Entries.clear();
		if (this->options.vpk_generateMD5Entries) {
			for (const auto& [tDir, tEntries] : this->entries) {
				for (const auto& tEntry : tEntries) {
					// Believe it or not this should be safe to call by now
					auto binData = this->readEntry(tEntry);
					if (!binData) {
						continue;
					}
					MD5Entry md5Entry{};
					md5Entry.archiveIndex = tEntry.vpk_archiveIndex;
					md5Entry.length = tEntry.length - tEntry.vpk_preloadedData.size();
					md5Entry.offset = tEntry.vpk_offset;
					md5Entry.checksum = md5(*binData);
					this->md5Entries.push_back(md5Entry);
				}
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
            outDir.seek_input(sizeof(Header1) + sizeof(Header2));
            std::vector<std::byte> treeData = outDir.read_bytes(this->header1.treeSize);
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
    outDir.seek_output(0);
    outDir.write(&this->header1);

    // v2 adds the MD5 hashes and file signature
    if (this->header1.version < 2) {
        return true;
    }

    outDir.write(&this->header2);

    // Add MD5 hashes
    outDir.seek_output(sizeof(Header1) + sizeof(Header2) + this->header1.treeSize + dirVPKEntryData.size());
    outDir.write_unsafe(this->md5Entries.data(), this->md5Entries.size());
    outDir.write_bytes(this->footer2.treeChecksum);
    outDir.write_bytes(this->footer2.md5EntriesChecksum);
    outDir.write_bytes(this->footer2.wholeFileChecksum);

    // The signature section is not present
    return true;
}

std::string VPK::getTruncatedFilestem() const {
	std::string filestem = this->getFilestem();
	// This indicates it's a dir VPK, but some people ignore this convention...
	if (filestem.length() >= 4 && filestem.substr(filestem.length() - 4) == "_dir") {
		filestem = filestem.substr(0, filestem.length() - 4);
	}
	return filestem;
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

std::uint32_t VPK::getHeaderLength() const {
	if (this->header1.version < 2) {
		return sizeof(Header1);
	}
	return sizeof(Header1) + sizeof(Header2);
}
