#include <vpktool/VPK.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <utility>

#include <vpktool/detail/CRC.h>

using namespace vpktool;
using namespace vpktool::detail;

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

    if (vpk.header2.otherMD5SectionSize != 48)
        return false;

    vpk.footer2.treeChecksum = vpk.reader.readBytes<16>();
    vpk.footer2.md5EntriesChecksum = vpk.reader.readBytes<16>();
    vpk.footer2.wholeFileChecksum = vpk.reader.readBytes<16>();

    vpk.footer2.cs2VPK = false;
    if (!vpk.header2.signatureSectionSize) {
        return true;
    }

    auto publicKeySize = vpk.reader.read<std::int32_t>();
    if (vpk.header2.signatureSectionSize == 20 && publicKeySize == VPK_ID) {
        vpk.footer2.cs2VPK = true;
        return true;
    }

    vpk.footer2.publicKey = vpk.reader.readBytes(publicKeySize);
    vpk.footer2.signature = vpk.reader.readBytes(vpk.reader.read<std::int32_t>());

    return true;
}

std::optional<VPKEntry> VPK::findEntry(const std::string& filename_) const {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    return this->findEntry(dir, name);
}

std::optional<VPKEntry> VPK::findEntry(const std::string& directory, const std::string& filename_) const {
    if (!this->entries.count(directory)) {
        // There are no files with this extension
        return std::nullopt;
    }

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
    for (const VPKEntry& entry : this->entries.at(dir)) {
        if (entry.filename == filename_) {
            return entry;
        }
    }

    return std::nullopt;
}

std::vector<std::byte> VPK::readBinaryEntry(const VPKEntry& entry) const {
    std::vector<std::byte> output(entry.preloadedData.size() + entry.length, static_cast<std::byte>(0));

    if (!entry.preloadedData.empty()) {
        std::copy(entry.preloadedData.begin(), entry.preloadedData.end(), output.begin());
    }

    if (entry.length == entry.preloadedData.size()) {
        return output;
    }

    if (entry.archiveIndex != VPK_DIR_INDEX) {
        FileStream stream{this->filename + '_' + padArchiveIndex(entry.archiveIndex) + ".vpk"};
        if (!stream) {
            // Error!
            return {};
        }
        stream.seekInput(static_cast<long>(entry.offset));
        auto bytes = stream.readBytes(entry.length);
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.preloadedData.size()));
    } else if (!filename.empty()) {
        FileStream stream{this->filename + ".vpk"};
        if (!stream) {
            // Error!
            return {};
        }
        stream.seekInput(static_cast<long>(this->getHeaderLength() + this->header1.treeSize + entry.offset));
        auto bytes = stream.readBytes(entry.length);
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.preloadedData.size()));
    } else {
        // Loaded from memory, but file is not in the directory VPK!
        return {};
    }

    return output;
}

std::string VPK::readTextEntry(const VPKEntry& entry) const {
    auto bytes = this->readBinaryEntry(entry);
    std::string out;
    for (auto byte : bytes) {
        if (byte == std::byte(0))
            break;
        out += static_cast<char>(byte);
    }
    return out;
}

void VPK::addEntry(const std::string& filename_, const std::string& pathToFile, bool preload) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    this->addEntry(dir, name, pathToFile, preload);
}

void VPK::addEntry(const std::string& directory, const std::string& filename_, const std::string& pathToFile, bool preload) {
    std::ifstream file(pathToFile, std::ios::binary);
    file.unsetf(std::ios::skipws);

    std::streampos fileSize;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> data;
    data.reserve(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    this->addBinaryEntry(directory, filename_, std::move(data), preload);
}

void VPK::addBinaryEntry(const std::string& filename_, std::vector<std::byte>&& buffer, bool preload) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    this->addBinaryEntry(dir, name, std::forward<std::vector<std::byte>>(buffer), preload);
}

void VPK::addBinaryEntry(const std::string& directory, const std::string& filename_, std::vector<std::byte>&& buffer, bool preload) {
    auto dir = directory;
    if (dir.empty()) {
        dir = " ";
    } else {
        std::replace(dir.begin(), dir.end(), '\\', '/');
    }

    // Offset and archive index will be set when the VPK is baked
    VPKEntry entry{};
    entry.filename = filename_;

    std::filesystem::path filePath{entry.filename};
    entry.filenamePair = std::make_pair(filePath.stem().string(), filePath.extension().string());

    entry.crc32 = computeCRC(buffer);
    entry.length = buffer.size();
    entry.offset = -1;
    entry.archiveIndex = -1;

    if (preload) {
        // Maximum preloaded data size is 1kb
        std::vector<std::byte>::size_type bufSize = buffer.size() > 1023 ? 1023 : buffer.size();
        entry.preloadedData.resize(bufSize);
        std::memcpy(entry.preloadedData.data(), buffer.data(), bufSize);
        buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::streamsize>(bufSize));
        entry.length -= entry.preloadedData.size();
    }

    if (!this->unbakedEntries.count(dir)) {
        this->unbakedEntries[dir] = {};
    }
    this->unbakedEntries.at(dir).emplace_back(entry, std::move(buffer));
}

void VPK::addBinaryEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, bool preload) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    this->addBinaryEntry(dir, name, buffer, bufferLen, preload);
}

void VPK::addBinaryEntry(const std::string& directory, const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, bool preload) {
    std::vector<std::byte> data;
    data.resize(bufferLen);
    std::memcpy(data.data(), buffer, bufferLen);
    this->addBinaryEntry(directory, filename_, std::move(data), preload);
}

void VPK::addTextEntry(const std::string& filename_, const std::string& text, bool preload) {
    const auto [dir, name] = splitFileNameAndParentDir(filename_);
    this->addTextEntry(dir, name, text, preload);
}

void VPK::addTextEntry(const std::string& directory, const std::string& filename_, const std::string& text, bool preload) {
    std::vector<std::byte> data;
    data.reserve(text.size());
    std::transform(text.begin(), text.end(), std::back_inserter(data), [](char c) {
        return std::byte(c);
    });
    this->addBinaryEntry(directory, filename_, std::move(data), preload);
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
                if (it->first.filename == filename_) {
                    unbakedEntryVec.erase(it);
                    return true;
                }
            }
        }
    }

    // If it's not in regular entries either you can't remove it!
    if (!this->entries.count(dir))
        return false;

    for (auto it = this->entries.at(dir).begin(); it != this->entries.at(dir).end();) {
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
    for (auto &[tDir, tEntries]: this->unbakedEntries) {
        for (auto &[tEntry, tData]: tEntries) {
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

    // Temporarily store file data that's stored in the directory VPK since it's getting overwritten
    std::vector<std::byte> dirVPKEntryData;
    std::size_t newDirEntryOffset = 0;
    for (auto& [tDir, tEntries] : this->entries) {
        for (auto& tEntry : tEntries) {
            if (tEntry.archiveIndex == VPK_DIR_INDEX) {
                auto binData = VPK::readBinaryEntry(tEntry);
                auto binDataSize = binData.size();
                dirVPKEntryData.reserve(dirVPKEntryData.size() + binDataSize);
                dirVPKEntryData.insert(dirVPKEntryData.end(), binData.begin(), binData.end());

                tEntry.offset = newDirEntryOffset;
                newDirEntryOffset += binDataSize;
            }
        }
    }

    // Get the file output paths
    std::string outputFolder;
    if (!outputFolder_.empty()) {
        outputFolder = outputFolder_;
        if (outputFolder.at(outputFolder.length() - 1) == '/' || outputFolder.at(outputFolder.length() - 1) == '\\') {
            outputFolder.pop_back();
        }
    } else {
        outputFolder = this->fullPath;
        std::replace(outputFolder.begin(), outputFolder.end(), '\\', '/');
        auto lastSlash = outputFolder.rfind('/');
        if (lastSlash != std::string::npos) {
            outputFolder = filename.substr(0, lastSlash);
        }
    }

    // todo: copy archive files if not saving to the same directory

    auto dirVPKFilePath = outputFolder + '/' + this->getPrettyFileName().data() + ".vpk";
    FileStream outDir{dirVPKFilePath, FILESTREAM_OPT_CREATE_IF_NONEXISTENT | FILESTREAM_OPT_TRUNCATE};

    std::unique_ptr<FileStream> outArchive = nullptr;
    if (!this->unbakedEntries.empty()) {
        outArchive = std::make_unique<FileStream>(outputFolder + '/' + this->getPrettyFileName().data() + '_' + padArchiveIndex(this->numArchives++) + ".vpk");
    }

    // Dummy header
    outDir.seekInput(0);
    outDir.seekOutput(0);
    outDir.write(&this->header1);
    if (this->header1.version == 2) {
        outDir.write(&this->header2);
    }

    // Sizes and offsets
    auto headerSize = sizeof(Header1) + (this->header1.version == 2 ? sizeof(Header2) : 0);
    std::size_t newEntryArchiveOffset = 0;

    // File tree data
    for (auto& [ext, tDirs] : temp) {
        outDir.write(ext);
        outDir.write('\0');

        for (auto& [dir, tEntries] : tDirs) {
            outDir.write(dir.length() > 0 ? dir : " ");
            outDir.write('\0');

            for (auto* entry : tEntries) {
                // Calculate entry offset if it's in the new archive and upload the data
                if (outArchive && entry->archiveIndex == static_cast<std::uint16_t>(-1)) {
                    entry->archiveIndex = this->numArchives;
                    entry->offset = newEntryArchiveOffset;
                    for (const auto& [unbakedEntry, entryData] : this->unbakedEntries.at(dir)) {
                        if (entry->filename == unbakedEntry.filename) {
                            outArchive->writeBytes(entryData);
                            newEntryArchiveOffset += entryData.size();
                            break;
                        }
                    }
                }

                outDir.write(entry->filenamePair.first);
                outDir.write('\0');
                outDir.write(entry->crc32);
                outDir.write(static_cast<std::uint16_t>(entry->preloadedData.size()));
                outDir.write(entry->archiveIndex);
                outDir.write(entry->offset);
                outDir.write(entry->length);
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

    // Merge unbaked into baked entries
    for (const auto& [tDir, tUnbakedEntriesAndData] : this->unbakedEntries) {
        for (const auto& [tUnbakedEntry, tData] : tUnbakedEntriesAndData) {
            if (!this->entries.count(tDir)) {
                this->entries[tDir] = {};
            }
            this->entries.at(tDir).push_back(tUnbakedEntry);
        }
    }
    this->unbakedEntries.clear();

    // Get sizes
    auto treeSize = outDir.tellOutput() - headerSize;
    auto afterFileData = outDir.tellOutput();
    auto fileDataSize = afterFileData - treeSize - headerSize;

    this->header1.treeSize = treeSize;

    this->header2.fileDataSectionSize = fileDataSize;
    this->header2.archiveMD5SectionSize = 0;
    this->header2.otherMD5SectionSize = 48;
    this->header2.signatureSectionSize = this->footer2.cs2VPK ? 20 : 0;

    // todo: calculate md5 entries
    this->md5Entries.clear();
    std::memset(this->footer2.treeChecksum.data(), 0, this->footer2.treeChecksum.size());
    std::memset(this->footer2.md5EntriesChecksum.data(), 0, this->footer2.md5EntriesChecksum.size());
    std::memset(this->footer2.wholeFileChecksum.data(), 0, this->footer2.wholeFileChecksum.size());

    // We can't recalculate the signature without the private key
    this->footer2.publicKey.clear();
    this->footer2.signature.clear();

    // Write new headers
    outDir.seekOutput(0);
    outDir.write(&this->header1);
    if (this->header1.version == 2) {
        outDir.write(&this->header2);
    }

    // Put files copied from the dir archive back
    outDir.seekOutput(sizeof(Header1) + sizeof(Header2) + treeSize);
    outDir.writeBytes(dirVPKEntryData);

    // v2 adds the MD5 hashes and file signature
    if (this->header1.version != 2) {
        return true;
    }

    // Add MD5 hashes
    outDir.write(this->md5Entries.data(), this->md5Entries.size());
    outDir.writeBytes(this->footer2.treeChecksum);
    outDir.writeBytes(this->footer2.md5EntriesChecksum);
    outDir.writeBytes(this->footer2.wholeFileChecksum);

    // The signature section is not present unless it's a CS2 vpk
    if (this->footer2.cs2VPK) {
        outDir.write(static_cast<std::int32_t>(VPK_ID));
        // Pad it with 16 bytes of junk, who knows what Valve wants here
        outDir.writeBytes(std::array<std::byte, 16>{});
    }

    return true;
}
