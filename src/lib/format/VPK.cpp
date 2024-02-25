#include <vpkedit/format/VPK.h>

#include <filesystem>

#include <MD5.h>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

constexpr std::uint32_t VPK_FLAG_REUSING_CHUNK = 0x1;

namespace {

std::string removeVPKAndOrDirSuffix(const std::string& path, bool isFPX) {
	std::string filename = path;
	if (filename.length() >= 4 && filename.substr(filename.length() - 4) == (isFPX ? FPX_EXTENSION : VPK_EXTENSION)) {
		filename = filename.substr(0, filename.length() - 4);
	}

	// This indicates it's a dir VPK, but some people ignore this convention...
	// It should fail later if it's not a proper dir VPK
	if (filename.length() >= 4 && filename.substr(filename.length() - 4) == (isFPX ? FPX_DIR_SUFFIX : VPK_DIR_SUFFIX)) {
		filename = filename.substr(0, filename.length() - 4);
	}

	return filename;
}

std::string padArchiveIndex(int num) {
	static constexpr int WIDTH = 3;
    auto numStr = std::to_string(num);
    return std::string(WIDTH - std::min<std::string::size_type>(WIDTH, numStr.length()), '0') + numStr;
}

bool isFPX(const VPK* vpk) {
	return vpk->getType() == PackFileType::FPX;
}

} // namespace

VPK::VPK(const std::string& fullFilePath_, PackFileOptions options_)
        : PackFile(fullFilePath_, options_) {
	this->type = PackFileType::VPK;
}

std::unique_ptr<PackFile> VPK::createEmpty(const std::string& path, PackFileOptions options) {
	{
        FileStream stream{path, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};

        Header1 header1{};
        header1.signature = VPK_SIGNATURE;
        header1.version = options.vpk_version;
        header1.treeSize = 1;
        stream.write(header1);

        if (options.vpk_version != 1) {
            Header2 header2{};
            header2.fileDataSectionSize = 0;
            header2.archiveMD5SectionSize = 0;
            header2.otherMD5SectionSize = 0;
            header2.signatureSectionSize = 0;
            stream.write(header2);
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
	auto vpk = VPK::openInternal(path, options, callback);
	if (!vpk && path.length() > 8) {
		// If it just tried to load a numbered archive, let's try to load the directory VPK
		if (auto dirPath = path.substr(0, path.length() - 8) + VPK_DIR_SUFFIX.data() + std::filesystem::path(path).extension().string(); std::filesystem::exists(dirPath)) {
			vpk = VPK::openInternal(dirPath, options, callback);
		}
	}
	return vpk;
}

std::unique_ptr<PackFile> VPK::openInternal(const std::string& path, PackFileOptions options, const Callback& callback) {
    if (!std::filesystem::exists(path)) {
        // File does not exist
        return nullptr;
    }

	auto* vpk = new VPK{path, options};
    auto packFile = std::unique_ptr<PackFile>(vpk);

	FileStream reader{vpk->fullFilePath};
    reader.seekInput(0);
    reader.read(vpk->header1);
    if ((!::isFPX(vpk) && vpk->header1.signature != VPK_SIGNATURE) ||
		(::isFPX(vpk) && vpk->header1.signature != FPX_SIGNATURE)) {
        // File is not a VPK or FPX
        return nullptr;
    }
	if (!::isFPX(vpk)) {
		if (vpk->header1.version == 2) {
			reader.read(vpk->header2);
		} else if (vpk->header1.version != 1) {
			// Apex Legends, Titanfall, etc. are not supported
			return nullptr;
		}
	} else if (vpk->header1.version != 10) {
		// Only support v10 FPX files
		return nullptr;
	}
	vpk->options.vpk_version = vpk->header1.version;

    // Extensions
    while (true) {
        std::string extension;
        reader.read(extension);
        if (extension.empty())
            break;

        // Directories
        while (true) {
            std::string directory;
            reader.read(directory);
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
                reader.read(entryName);
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

                reader.read(entry.crc32);
                auto preloadedDataSize = reader.read<std::uint16_t>();
                reader.read(entry.vpk_archiveIndex);
                entry.offset = reader.read<std::uint32_t>();
	            entry.length = reader.read<std::uint32_t>();

                if (reader.read<std::uint16_t>() != VPK_ENTRY_TERM) {
                    // Invalid terminator!
                    return nullptr;
                }

                if (preloadedDataSize > 0) {
                    entry.vpk_preloadedData = reader.readBytes(preloadedDataSize);
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
    reader.seekInput(vpk->header2.fileDataSectionSize, std::ios_base::cur);

    if (vpk->header2.archiveMD5SectionSize % sizeof(MD5Entry) != 0)
        return nullptr;

    vpk->md5Entries.clear();
    unsigned int entryNum = vpk->header2.archiveMD5SectionSize / sizeof(MD5Entry);
    for (unsigned int i = 0; i < entryNum; i++)
        vpk->md5Entries.push_back(reader.read<MD5Entry>());

    if (vpk->header2.otherMD5SectionSize != 48)
	    // This should always be 48
        return packFile;

	vpk->footer2.treeChecksum = reader.readBytes<16>();
	vpk->footer2.md5EntriesChecksum = reader.readBytes<16>();
	vpk->footer2.wholeFileChecksum = reader.readBytes<16>();

    if (!vpk->header2.signatureSectionSize) {
        return packFile;
    }

    auto publicKeySize = reader.read<std::int32_t>();
    if (vpk->header2.signatureSectionSize == 20 && publicKeySize == VPK_SIGNATURE) {
        // CS2 beta VPK, ignore it
        return packFile;
    }

    vpk->footer2.publicKey = reader.readBytes(publicKeySize);
    vpk->footer2.signature = reader.readBytes(reader.read<std::int32_t>());

    return packFile;
}

std::vector<std::string> VPK::verifyEntryChecksums() const {
	return this->verifyEntryChecksumsUsingCRC32();
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
        FileStream stream{this->getTruncatedFilepath() + '_' + ::padArchiveIndex(entry.vpk_archiveIndex) + (::isFPX(this) ? FPX_EXTENSION : VPK_EXTENSION).data()};
        if (!stream) {
            return std::nullopt;
        }
        stream.seekInput(entry.offset);
        auto bytes = stream.readBytes(entry.length - entry.vpk_preloadedData.size());
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.vpk_preloadedData.size()));
    } else {
		// Stored in this directory VPK
        FileStream stream{this->fullFilePath};
        if (!stream) {
            return std::nullopt;
        }
        stream.seekInput(this->getHeaderLength() + this->header1.treeSize + entry.offset);
        auto bytes = stream.readBytes(entry.length - entry.vpk_preloadedData.size());
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.vpk_preloadedData.size()));
    }

    return output;
}

Entry& VPK::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	auto filename = filename_;
	if (!this->isCaseSensitive()) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

	entry.path = filename;
	entry.crc32 = ::computeCRC32(buffer);
	entry.length = buffer.size();

	// Offset will be reset when it's baked, assuming we're not replacing an existing chunk (when flags = 1)
	entry.flags = 0;
	entry.offset = 0;
	entry.vpk_archiveIndex = options_.vpk_saveToDirectory ? VPK_DIR_INDEX : this->numArchives;
	if (!options_.vpk_saveToDirectory && !this->freedChunks.empty()) {
		std::int64_t bestChunkIndex = -1;
		std::size_t currentChunkGap = SIZE_MAX;
		for (std::int64_t i = 0; i < this->freedChunks.size(); i++) {
			if (
				(bestChunkIndex < 0 && this->freedChunks[i].length >= entry.length) ||
				(bestChunkIndex >= 0 && this->freedChunks[i].length >= entry.length && (this->freedChunks[i].length - entry.length) < currentChunkGap)
			) {
				bestChunkIndex = i;
				currentChunkGap = this->freedChunks[i].length - entry.length;
			}
		}
		if (bestChunkIndex >= 0) {
			entry.flags |= VPK_FLAG_REUSING_CHUNK;
			entry.offset = this->freedChunks[bestChunkIndex].offset;
			entry.vpk_archiveIndex = this->freedChunks[bestChunkIndex].archiveIndex;
			this->freedChunks.erase(this->freedChunks.begin() + bestChunkIndex);
			if (currentChunkGap < SIZE_MAX && currentChunkGap > 0) {
				// Add the remaining free space as a free chunk
				this->freedChunks.push_back({entry.offset + entry.length, currentChunkGap, entry.vpk_archiveIndex});
			}
		}
	}

	if (options_.vpk_preloadBytes > 0) {
		auto clampedPreloadBytes = std::clamp(options_.vpk_preloadBytes, 0u, buffer.size() > VPK_MAX_PRELOAD_BYTES ? VPK_MAX_PRELOAD_BYTES : static_cast<std::uint32_t>(buffer.size()));
		entry.vpk_preloadedData.resize(clampedPreloadBytes);
		std::memcpy(entry.vpk_preloadedData.data(), buffer.data(), clampedPreloadBytes);
		buffer.erase(buffer.begin(), buffer.begin() + clampedPreloadBytes);
	}

	// Now that archive index is calculated for this entry, check if it needs to be incremented
	if (!options_.vpk_saveToDirectory && !(entry.flags & VPK_FLAG_REUSING_CHUNK)) {
		entry.offset = this->currentlyFilledChunkSize;
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

bool VPK::removeEntry(const std::string& filename_) {
	if (auto entry = this->findEntry(filename_); entry && (!entry->unbaked || entry->flags & VPK_FLAG_REUSING_CHUNK)) {
		this->freedChunks.push_back({entry->offset, entry->length, entry->vpk_archiveIndex});
	}
	return PackFile::removeEntry(filename_);
}

bool VPK::bake(const std::string& outputDir_, const Callback& callback) {
	// Get the proper file output folder
	std::string outputDir = this->getBakeOutputDir(outputDir_);
	std::string outputPath = outputDir + '/' + this->getFilename();

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

                tEntry.offset = newDirEntryOffset;
                newDirEntryOffset += tEntry.length - tEntry.vpk_preloadedData.size();
            }
        }
    }

	// Helper
	const auto getArchiveFilename = [this](const std::string& filename_, int archiveIndex) {
		std::string out{filename_ + '_' + ::padArchiveIndex(archiveIndex) + (::isFPX(this) ? FPX_EXTENSION : VPK_EXTENSION).data()};
		::normalizeSlashes(out);
		return out;
	};

    // Copy external binary blobs to the new dir
    if (!outputDir.empty()) {
        for (int archiveIndex = 0; archiveIndex < this->numArchives; archiveIndex++) {
			std::string from = getArchiveFilename(this->getTruncatedFilepath(), archiveIndex);
	        if (!std::filesystem::exists(from)) {
		        continue;
	        }
	        std::string dest = getArchiveFilename(outputDir + '/' + this->getTruncatedFilestem(), archiveIndex);
			if (from == dest) {
				continue;
			}
			std::filesystem::copy_file(from, dest, std::filesystem::copy_options::overwrite_existing);
        }
    }

    FileStream outDir{outputPath, FILESTREAM_OPT_READ | FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
    outDir.seekInput(0);
    outDir.seekOutput(0);

	// Dummy header
    outDir.write(this->header1);
    if (this->header1.version == 2) {
        outDir.write(this->header2);
    }

    // File tree data
    for (auto& [ext, tDirs] : temp) {
        outDir.write(ext);

        for (auto& [dir, tEntries] : tDirs) {
            outDir.write(!dir.empty() ? dir : " ");

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
						entry->offset = dirVPKEntryData.size();
					} else if (entry->vpk_archiveIndex != VPK_DIR_INDEX && (entry->flags & VPK_FLAG_REUSING_CHUNK)) {
						// The entry is replacing pre-existing data in a VPK archive
						auto archiveFilename = getArchiveFilename(::removeVPKAndOrDirSuffix(outputPath, ::isFPX(this)), entry->vpk_archiveIndex);
                        FileStream stream{archiveFilename, FILESTREAM_OPT_READ | FILESTREAM_OPT_WRITE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
						stream.seekOutput(entry->offset);
                        stream.writeBytes(entryData);
                    } else if (entry->vpk_archiveIndex != VPK_DIR_INDEX) {
						// The entry is being appended to a newly created VPK archive
						auto archiveFilename = getArchiveFilename(::removeVPKAndOrDirSuffix(outputPath, ::isFPX(this)), entry->vpk_archiveIndex);
						entry->offset = std::filesystem::exists(archiveFilename) ? std::filesystem::file_size(archiveFilename) : 0;
                        FileStream stream{archiveFilename, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_APPEND | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
                        stream.writeBytes(entryData);
                    } else {
						// The entry will be added to the directory VPK
                        entry->offset = dirVPKEntryData.size();
                        dirVPKEntryData.insert(dirVPKEntryData.end(), entryData.data(), entryData.data() + entryData.size());
                    }

					// Clear flags
					entry->flags = 0;
                }

                outDir.write(entry->getStem());
                outDir.write(entry->crc32);
                outDir.write(static_cast<std::uint16_t>(entry->vpk_preloadedData.size()));
                outDir.write(entry->vpk_archiveIndex);
                outDir.write(static_cast<std::uint32_t>(entry->offset));
                outDir.write(static_cast<std::uint32_t>(entry->length - entry->vpk_preloadedData.size()));
                outDir.write(VPK_ENTRY_TERM);

                if (!entry->vpk_preloadedData.empty()) {
                    outDir.writeBytes(entry->vpk_preloadedData);
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
	this->mergeUnbakedEntries();

    // Calculate Header1
    this->header1.treeSize = outDir.tellOutput() - dirVPKEntryData.size() - this->getHeaderLength();

    // VPK v2 stuff
    if (this->header1.version == 2) {
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
					md5Entry.offset = tEntry.offset;
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
    outDir.write(this->header1);

    // v2 adds the MD5 hashes and file signature
    if (this->header1.version != 2) {
	    PackFile::setFullFilePath(outputDir);
        return true;
    }

    outDir.write(this->header2);

    // Add MD5 hashes
    outDir.seekOutput(sizeof(Header1) + sizeof(Header2) + this->header1.treeSize + dirVPKEntryData.size());
    outDir.write(this->md5Entries);
    outDir.writeBytes(this->footer2.treeChecksum);
    outDir.writeBytes(this->footer2.md5EntriesChecksum);
    outDir.writeBytes(this->footer2.wholeFileChecksum);

    // The signature section is not present
	PackFile::setFullFilePath(outputDir);
    return true;
}

std::string VPK::getTruncatedFilestem() const {
	std::string filestem = this->getFilestem();
	// This indicates it's a dir VPK, but some people ignore this convention...
	if (filestem.length() >= 4 && filestem.substr(filestem.length() - 4) == (::isFPX(this) ? FPX_DIR_SUFFIX : VPK_DIR_SUFFIX)) {
		filestem = filestem.substr(0, filestem.length() - 4);
	}
	return filestem;
}

std::vector<Attribute> VPK::getSupportedEntryAttributes() const {
	using enum Attribute;
	return {LENGTH, VPK_PRELOADED_DATA_LENGTH, VPK_ARCHIVE_INDEX, CRC32};
}

VPK::operator std::string() const {
	return PackFile::operator std::string() +
		" | Version v" + std::to_string(this->header1.version);
}

std::uint32_t VPK::getVersion() const {
    return this->header1.version;
}

void VPK::setVersion(std::uint32_t version) {
    if (::isFPX(this) || version == this->header1.version) {
        return;
    }
    this->header1.version = version;
	this->options.vpk_version = version;

    // Clearing these isn't necessary, but might as well
    this->header2 = Header2{};
    this->footer2 = Footer2{};
    this->md5Entries.clear();
}

std::uint32_t VPK::getHeaderLength() const {
	if (this->header1.version != 2) {
		return sizeof(Header1);
	}
	return sizeof(Header1) + sizeof(Header2);
}

FPX::FPX(const std::string& fullFilePath_, PackFileOptions options_)
		: VPK(fullFilePath_, options_) {
	this->type = PackFileType::FPX;
}

std::unique_ptr<PackFile> FPX::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	auto fpx = FPX::openInternal(path, options, callback);
	if (!fpx && path.length() > 8) {
		// If it just tried to load a numbered archive, let's try to load the directory FPX
		if (auto dirPath = path.substr(0, path.length() - 8) + FPX_DIR_SUFFIX.data() + std::filesystem::path(path).extension().string(); std::filesystem::exists(dirPath)) {
			fpx = FPX::openInternal(dirPath, options, callback);
		}
	}
	return fpx;
}
