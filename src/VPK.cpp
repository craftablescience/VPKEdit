/*
 * Contents of this file are part of a C++ port of https://github.com/SteamDatabase/ValvePak, which is licensed under MIT.
 * This port adds VPK1 support and removes CRC checks.
 */

#include <vpktool/VPK.h>

#include <algorithm>
#include <iterator>
#include <filesystem>

using namespace vpktool;

VPK::VPK(const std::string& vpkName) : reader(vpkName) {
    this->fileName = vpkName;

    if (this->fileName.length() >= 4 && this->fileName.substr(this->fileName.length() - 4) == ".vpk") {
        this->fileName = this->fileName.substr(0, this->fileName.length() - 4);
    }

    if (this->fileName.length() >= 4 && this->fileName.substr(this->fileName.length() - 4) == "_dir") {
        this->isDirVPK = true;
        this->fileName = this->fileName.substr(0, this->fileName.length() - 4);
    }

    this->reader.seek(0);
    if (this->reader.read<std::uint32_t>() != 0x55AA1234) {
        // File is not a VPK
        this->isValid = false;
        return;
    }

    this->version  = this->reader.read<std::uint32_t>();
    this->treeSize = this->reader.read<std::uint32_t>();

    if (this->version == 2) {
        this->fileDataSectionSize   = this->reader.read<std::uint32_t>();
        this->archiveMD5SectionSize = this->reader.read<std::uint32_t>();
        this->otherMD5SectionSize   = this->reader.read<std::uint32_t>();
        this->signatureSectionSize  = this->reader.read<std::uint32_t>();
    } else if (this->version == 0x00030002 || this->version != 1) {
        // Apex Legends, Titanfall, etc. are not supported
        this->isValid = false;
        return;
    }

    this->headerSize = this->reader.tell();

    // Read entries
    while (true) {
        std::string typeName = this->reader.readString();
        if (typeName.empty())
            break;

        std::vector<VPKEntry> vpkEntries;

        // Directories
        while (true) {
            std::string directoryName = this->reader.readString();
            if (directoryName.empty())
                break;

            // Files
            while (true) {
                std::string filename = this->reader.readString();
                if (filename.empty())
                    break;

                VPKEntry entry{};
                entry.fileName = filename;
                entry.directoryName = directoryName;
                entry.typeName = typeName;
                entry.crc32 = this->reader.read<std::uint32_t>();
                auto smallDataSize = this->reader.read<std::uint16_t>();
                entry.archiveIndex = this->reader.read<std::uint16_t>();
                entry.offset = this->reader.read<std::uint32_t>();
                entry.length = this->reader.read<std::uint32_t>();

                if (this->reader.read<std::uint16_t>() != 0xFFFF) {
                    // Invalid terminator! Not much we can do...
                }
                if (smallDataSize > 0)
                    entry.smallData = this->reader.readBytes(smallDataSize);
                vpkEntries.push_back(entry);
            }
        }
        this->entries[typeName] = vpkEntries;
    }

    // Read VPK2-specific data
    if (this->version == 2) {
        // Skip over file data, if any
        this->reader.seek(static_cast<long>(this->fileDataSectionSize), std::ios_base::cur);

        this->archiveMD5Entries.clear();
        if (this->archiveMD5SectionSize == 0)
            return;
        // 28 is sizeof(VPK_MD5SectionEntry), which is int + int + int + 16 chars
        unsigned int entryNum = this->archiveMD5SectionSize / 28;
        for (unsigned int i = 0; i < entryNum; i++)
            this->archiveMD5Entries.emplace_back(
                    this->reader.read<std::uint32_t>(),
                    this->reader.read<std::uint32_t>(),
                    this->reader.read<std::uint32_t>(),
                    this->reader.readBytes(16)
            );

        //if (this->otherMD5SectionSize != 48)
        //    Logger::log(LogType::LOG_ERROR, "VPK", TRF("error.vpk.md5_section_bad_size", this->otherMD5SectionSize));
        this->treeChecksum              = this->reader.readBytes(16);
        this->archiveMD5EntriesChecksum = this->reader.readBytes(16);
        this->wholeFileChecksum         = this->reader.readBytes(16);

        if (this->signatureSectionSize == 0)
            return;
        this->publicKey = this->reader.readBytes(this->reader.read<std::int32_t>());
        this->signature = this->reader.readBytes(this->reader.read<std::int32_t>());
    }
}

VPKEntry VPK::findEntry(const std::string& filePath) const {
    std::string filepathCopy = filePath;
    std::replace(filepathCopy.begin(), filepathCopy.end(), '\\', '/');
    auto lastSeparator = filepathCopy.rfind('/');
    auto directory = lastSeparator != std::string::npos ? filepathCopy.substr(0, lastSeparator) : "";
    auto fileName_ = filepathCopy.substr((lastSeparator + 1));
    return this->findEntry(directory, fileName_);
}

VPKEntry VPK::findEntry(const std::string& directory, const std::string& fileName_) const {
    std::string extension;
    std::string filename;
    if (auto dot = fileName_.rfind('.'); dot != std::string::npos) {
        extension = fileName_.substr(dot + 1);
        filename = fileName_.substr(0, dot);
    } else {
        // Valve uses a space for missing extensions
        extension = " ";
        filename = fileName_;
    }
    return this->findEntry(directory, filename, extension);
}

VPKEntry VPK::findEntry(const std::string& directory, const std::string& fileName_, const std::string& extension) const {
    if (this->entries.count(extension) == 0) {
        // There are no files with this extension
        return {};
    }
    std::string dir = directory;
    if (!dir.empty()) {
        std::replace(dir.begin(), dir.end(), '\\', '/');
        if (dir.length() > 1 && dir.substr(0,1) == "/") {
            dir = dir.substr(1);
        }
        if (dir.length() > 2 && dir.substr(dir.length() - 1) == "/") {
            dir = dir.substr(0, dir.length() - 2);
        }
    } else {
        // If the directory is empty after trimming, set it to a space to match Valve's behavior
        dir = " ";
    }
    for (const VPKEntry& entry : this->entries.at(extension)) {
        if (entry.directoryName == dir && entry.fileName == fileName_) {
            return entry;
        }
    }
    return {};
}

bool VPK::readEntry(const VPKEntry& entry, std::vector<byte>& output) const {
    output.clear();
    output.reserve(entry.smallData.size() + entry.length);

    if (!entry.smallData.empty())
        std::copy(entry.smallData.begin(), entry.smallData.end(), std::back_inserter(output));

    if (entry.length > 0) {
        if (entry.archiveIndex != 0x7FFF) {
            if (!this->isDirVPK) {
                return false;
            }
            char name[1024] {0};
            snprintf(name,1023, "%s_%03d.vpk", this->fileName.c_str(), entry.archiveIndex);
            FileInputStream stream{name};
            if (!stream) {
                return false;
            }
            stream.seek(static_cast<long>(entry.offset));
            output = stream.readBytes(entry.length);
        } else {
            FileInputStream stream{this->fileName + ".vpk"};
            if (!stream) {
                return false;
            }
            stream.seek(static_cast<long>(entry.offset) + static_cast<long>(this->headerSize) + static_cast<long>(this->treeSize));
            output = stream.readBytes(entry.length);
        }
        return true;
    }
    return false;
}

VPK::operator bool() const {
    return this->isValid;
}

bool VPK::operator!() const {
    return !this->operator bool();
}
