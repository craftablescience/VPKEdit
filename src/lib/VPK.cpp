#include <vpkedit/VPK.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <utility>

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

std::pair<std::string, std::string> splitFileNameAndParentDir(const std::string& filename) {
    auto name = filename;
    std::replace(name.begin(), name.end(), '\\', '/');

    auto lastSeparator = name.rfind('/');
    auto dir = lastSeparator != std::string::npos ? name.substr(0, lastSeparator) : "";
    name = filename.substr((lastSeparator + 1));

    return std::make_pair(dir, name);
}

inline std::string padArchiveIndex(int num, int width = 3) {
    auto numStr = std::to_string(num);
    return std::string(width - std::min<std::string::size_type>(width, numStr.length()), '0') + numStr;
}

} // namespace

VPK::VPK(FileStream&& reader_, std::string fullPath_, std::string filename_)
        : reader(std::move(reader_))
        , fullPath(std::move(fullPath_))
        , filename(std::move(filename_)) {}

VPK VPK::createEmpty(const std::string& path, std::uint32_t version, bool cs2VPK) {
    {
        FileStream stream{path, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};

        Header1 header1{};
        header1.signature = VPK_ID;
        header1.version = version;
        header1.treeSize = 1;
        stream.write(&header1);

        if (version != 1) {
            Header2 header2{};
            header2.fileDataSectionSize = 0;
            header2.archiveMD5SectionSize = 0;
            header2.otherMD5SectionSize = 0;
            header2.signatureSectionSize = cs2VPK ? 20 : 0;
            stream.write(&header2);

            stream.write('\0');

            if (cs2VPK) {
                // Copied from the bottom of this file
                stream.write(VPK_ID);
                // Pad it with 16 bytes of junk, who knows what Valve wants here
                std::array<std::byte, 16> junk{};
                junk[0] = static_cast<std::byte>(1); // ValvePak does this so we're doing it too
                stream.writeBytes(junk);
            }
        } else {
            stream.write('\0');
        }
    }
    return *VPK::open(path);
}

VPK VPK::createFromDirectory(const std::string& vpkPath, const std::string& directoryPath, std::uint32_t version, bool cs2VPK) {
    auto vpk = VPK::createEmpty(vpkPath, version, cs2VPK);
    auto absoluteDirPath = std::filesystem::absolute(std::filesystem::path(directoryPath)).string();
    for (const auto& file : std::filesystem::recursive_directory_iterator(directoryPath)) {
        if (file.is_directory()) {
            continue;
        }
        std::string entryPath = std::filesystem::absolute(file.path()).string().substr(directoryPath.length());
        if (entryPath.empty()) {
            continue;
        }
        while (entryPath.at(0) == '/' || entryPath.at(0) == '\\') {
            entryPath = entryPath.substr(1);
        }
        vpk.addEntry(entryPath, file.path().string());
    }
    vpk.bake();
    return vpk;
}

std::optional<VPK> VPK::open(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        // File does not exist
        return std::nullopt;
    }

    std::string filename = removeVPKAndOrDirSuffix(path);

    VPK vpk{FileStream{path}, path, filename};
    if (open(vpk)) {
        return vpk;
    }
    return std::nullopt;
}

std::optional<VPK> VPK::open(std::byte* buffer, std::uint64_t bufferLen) {
    VPK vpk{FileStream{buffer, bufferLen}, "", ""};
    if (open(vpk)) {
        return vpk;
    }
    return std::nullopt;
}

bool VPK::open(VPK& vpk) {
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
                    entry.filenamePair = std::make_pair(entryname, "");
                } else {
                    entry.filename = entryname + '.';
                    entry.filename += extension;
                    entry.filenamePair = std::make_pair(entryname, extension);
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
            }
        }
    }

    // If there are no archives, -1 will be incremented to 0
    vpk.numArchives++;

    // Read VPK2-specific data
    if (vpk.header1.version != 2)
        return true;

    // Skip over file data, if any
    vpk.reader.seekInput(static_cast<long>(vpk.header2.fileDataSectionSize), std::ios_base::cur);

    if (vpk.header2.archiveMD5SectionSize % sizeof(MD5Entry) != 0)
        return false;

    vpk.md5Entries.clear();
    unsigned int entryNum = vpk.header2.archiveMD5SectionSize / sizeof(MD5Entry);
    for (unsigned int i = 0; i < entryNum; i++)
        vpk.md5Entries.push_back(vpk.reader.read<MD5Entry>());

    if (vpk.header2.otherMD5SectionSize != 48 && vpk.header2.otherMD5SectionSize > 0)
        return false;

    if (vpk.header2.otherMD5SectionSize > 0) {
        // Assume it's 48
        vpk.footer2.treeChecksum = vpk.reader.readBytes<16>();
        vpk.footer2.md5EntriesChecksum = vpk.reader.readBytes<16>();
        vpk.footer2.wholeFileChecksum = vpk.reader.readBytes<16>();
    }

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
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    return this->findEntry(dir, name, includeUnbaked);
}

std::optional<VPKEntry> VPK::findEntry(const std::string& directory, const std::string& filename_, bool includeUnbaked) const {
    std::string dir = directory;
    if (!dir.empty()) {
        std::replace(dir.begin(), dir.end(), '\\', '/');
        if (dir.length() > 1 && dir.substr(0, 1) == "/") {
            dir = dir.substr(1);
        }
        if (dir.length() > 2 && dir.substr(dir.length() - 1) == "/") {
            dir = dir.substr(0, dir.length() - 2);
        }
    }
    if (this->entries.count(dir)) {
        for (const VPKEntry& entry : this->entries.at(dir)) {
            if (entry.filename == filename_) {
                return entry;
            }
        }
    }
    if (includeUnbaked && this->unbakedEntries.count(dir)) {
        for (const VPKEntry& unbakedEntry : this->unbakedEntries.at(dir)) {
            if (unbakedEntry.filename == filename_) {
                return unbakedEntry;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::vector<std::byte>> VPK::readBinaryEntry(const VPKEntry& entry) const {
    std::vector<std::byte> output(entry.length, static_cast<std::byte>(0));

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
                    std::copy(unbakedEntry.unbakedData.begin(), unbakedEntry.unbakedData.end(), output.begin() + static_cast<long long>(entry.preloadedData.size()));
                    return output;
                }
            }
        }
    } else if (entry.archiveIndex != VPK_DIR_INDEX) {
        FileStream stream{this->filename + '_' + padArchiveIndex(entry.archiveIndex) + ".vpk"};
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
        if (byte == std::byte(0))
            break;
        out += static_cast<char>(byte);
    }
    return out;
}

void VPK::addEntry(const std::string& filename_, const std::string& pathToFile, bool saveToDir, int preloadBytes) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    this->addEntry(dir, name, pathToFile, saveToDir, preloadBytes);
}

void VPK::addEntry(const std::string& directory, const std::string& filename_, const std::string& pathToFile, bool saveToDir, int preloadBytes) {
    FileStream stream{pathToFile};
    stream.seekInput(0);
    auto data = stream.readBytes(std::filesystem::file_size(pathToFile));
    this->addBinaryEntry(directory, filename_, std::move(data), saveToDir, preloadBytes);
}

void VPK::addBinaryEntry(const std::string& filename_, std::vector<std::byte>&& buffer, bool saveToDir, int preloadBytes) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    this->addBinaryEntry(dir, name, std::forward<std::vector<std::byte>>(buffer), saveToDir, preloadBytes);
}

void VPK::addBinaryEntry(const std::string& directory, const std::string& filename_, std::vector<std::byte>&& buffer, bool saveToDir, int preloadBytes) {
    auto dir = directory;
    std::replace(dir.begin(), dir.end(), '\\', '/');

    VPKEntry entry{};
    entry.unbaked = true;
    entry.filename = filename_;

    std::filesystem::path filePath{entry.filename};
    auto ext = filePath.extension().string();
    if (!ext.empty() && ext.at(0) == '.') {
        ext = ext.substr(1);
    }
    entry.filenamePair = std::make_pair(filePath.stem().string(), ext);

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
        buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::streamsize>(clampedPreloadBytes));
    }

    if (!this->unbakedEntries.count(dir)) {
        this->unbakedEntries[dir] = {};
    }

    entry.unbakedData = std::move(buffer);
    this->unbakedEntries.at(dir).push_back(std::move(entry));
}

void VPK::addBinaryEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, bool saveToDir, int preloadBytes) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    this->addBinaryEntry(dir, name, buffer, bufferLen, saveToDir, preloadBytes);
}

void VPK::addBinaryEntry(const std::string& directory, const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, bool saveToDir, int preloadBytes) {
    std::vector<std::byte> data;
    data.resize(bufferLen);
    std::memcpy(data.data(), buffer, bufferLen);
    this->addBinaryEntry(directory, filename_, std::move(data), saveToDir, preloadBytes);
}

void VPK::addTextEntry(const std::string& filename_, const std::string& text, bool saveToDir, int preloadBytes) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    this->addTextEntry(dir, name, text, saveToDir, preloadBytes);
}

void VPK::addTextEntry(const std::string& directory, const std::string& filename_, const std::string& text, bool saveToDir, int preloadBytes) {
    std::vector<std::byte> data;
    data.reserve(text.size());
    std::transform(text.begin(), text.end(), std::back_inserter(data), [](char c) {
        return std::byte(c);
    });
    this->addBinaryEntry(directory, filename_, std::move(data), saveToDir, preloadBytes);
}

bool VPK::removeEntry(const std::string& filename_) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    return this->removeEntry(dir, name);
}

bool VPK::removeEntry(const std::string& directory, const std::string& filename_) {
    auto dir = directory;
    std::replace(dir.begin(), dir.end(), '\\', '/');

    // Check unbaked entries first
    if (this->unbakedEntries.count(dir)) {
        for (auto& [preexistingDir, unbakedEntryVec] : this->unbakedEntries) {
            if (preexistingDir != dir) {
                continue;
            }
            for (auto it = unbakedEntryVec.begin(); it != unbakedEntryVec.end();) {
                if (it->filename == filename_) {
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
        if (it->filename == filename_) {
            this->entries.at(dir).erase(it);
            return true;
        }
    }
    return false;
}

bool VPK::bake(const std::string& outputFolder_) {
    // Loaded from memory
    if (this->fullPath.empty())
        return false;

    // Reconstruct data so we're not looping over it a ton of times
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<VPKEntry*>>> temp;

    for (auto& [tDir, tEntries] : this->entries) {
        for (auto& tEntry : tEntries) {
            std::string extension = tEntry.filenamePair.second.empty() ? " " : tEntry.filenamePair.second;
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
        for (VPKEntry& tEntry : tEntries) {
            std::string extension = tEntry.filenamePair.second.empty() ? " " : tEntry.filenamePair.second;
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
                dirVPKEntryData.insert(dirVPKEntryData.end(), (*binData).begin() + static_cast<std::vector<std::byte>::difference_type>(tEntry.preloadedData.size()), (*binData).end());

                tEntry.offset = newDirEntryOffset;
                newDirEntryOffset += tEntry.length - tEntry.preloadedData.size();
            }
        }
    }

    // Get the file output paths
    std::string outputFilename = std::filesystem::path(this->fullPath).filename().string();
    std::string outputFilenameNoExtension = this->getRealFileName();
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
        }
    }

    // Copy external binary blobs to the new dir
    auto dirVPKFilePath = outputFolder + '/' + outputFilename;
    if (!outputFolder_.empty()) {
        for (int archiveIndex = 0; archiveIndex < this->numArchives; archiveIndex++) {
            try {
                std::string dest = outputFolder + '/';
                dest.append(outputFilenameNoExtension + '_');
                dest.append(padArchiveIndex(archiveIndex));
                dest.append(".vpk");
                std::filesystem::copy(
                        this->filename + '_' + padArchiveIndex(archiveIndex) + ".vpk",
                        dest,
                        std::filesystem::copy_options::overwrite_existing);
            } catch (const std::filesystem::filesystem_error&) {}
        }
    }

    FileStream outDir{dirVPKFilePath, FILESTREAM_OPT_READ | FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
    std::unique_ptr<FileStream> outArchive = nullptr;

    // Dummy header
    outDir.seekInput(0);
    outDir.seekOutput(0);
    outDir.write(&this->header1);
    if (this->header1.version == 2) {
        outDir.write(&this->header2);
    }

    // File tree data
    std::size_t newEntryArchiveOffset = 0;
    for (auto& [ext, tDirs] : temp) {
        outDir.write(ext);
        outDir.write('\0');

        for (auto& [dir, tEntries] : tDirs) {
            outDir.write(dir.length() > 0 ? dir : " ");
            outDir.write('\0');

            for (auto* entry : tEntries) {
                // Calculate entry offset if it's unbaked and upload the data
                if (entry->unbaked) {
                    const std::byte* entryData = nullptr;
                    std::size_t entryDataSize = 0;
                    for (const VPKEntry& unbakedEntry : this->unbakedEntries.at(dir)) {
                        if (entry->filename != unbakedEntry.filename) {
                            continue;
                        }
                        entryData = entry->unbakedData.data();
                        entryDataSize = entry->unbakedData.size();
                        break;
                    }
                    if (!entryData) {
                        continue;
                    }
                    if (entry->length == entry->preloadedData.size()) {
                        // Override the archive index, no need for an archive VPK
                        entry->archiveIndex = VPK_DIR_INDEX;
                        entry->offset = dirVPKEntryData.size();
                    } else if (entry->archiveIndex != VPK_DIR_INDEX) {
                        entry->offset = newEntryArchiveOffset;
                        entry->archiveIndex = this->numArchives;
                        if (!outArchive) {
                            outArchive = std::make_unique<FileStream>(outputFolder + '/' + this->getPrettyFileName().data() + '_' + padArchiveIndex(this->numArchives) + ".vpk", FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT);
                            this->numArchives++;
                        }
                        outArchive->write(entryData, entryDataSize);
                        newEntryArchiveOffset += entryDataSize;
                    } else {
                        entry->offset = dirVPKEntryData.size();
                        dirVPKEntryData.insert(dirVPKEntryData.end(), entryData, entryData + entryDataSize);
                    }
                }

                outDir.write(entry->filenamePair.first);
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

std::string VPK::getRealFileName() const {
    return std::filesystem::path(this->fullPath).stem().string();
}
