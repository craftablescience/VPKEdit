/*
 * Contents of this file are part of a C++ port of https://github.com/SteamDatabase/ValvePak, which is licensed under MIT.
 * This port adds VPK1 support and removes CRC checks.
 */

#include <vpktool/VPK.h>

#include <algorithm>
#include <iterator>
#include <filesystem>
#include <utility>

using namespace vpktool;

VPK::VPK(InputStream&& reader_, std::string filename_, bool dirVPK)
        : reader(std::move(reader_))
        , filename(std::move(filename_))
        , isDirVPK(dirVPK) {}

std::optional<VPK> VPK::open(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        // File does not exist
        return std::nullopt;
    }

    std::string filename = path;
    if (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".vpk") {
        filename = filename.substr(0, filename.length() - 4);
    }

    bool dirVPK = false;
    if (filename.length() >= 4 && filename.substr(filename.length() - 4) == "_dir") {
        dirVPK = true;
        filename = filename.substr(0, filename.length() - 4);
    }

    VPK vpk{InputStream{path}, filename, dirVPK};
    if (open(vpk)) {
        return vpk;
    }
    return std::nullopt;
}

bool VPK::open(VPK& vpk) {
    vpk.reader.seek(0);
    vpk.reader.read(vpk.header1);
    if (vpk.header1.signature != 0x55AA1234) {
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
        auto extension = vpk.reader.read<std::string>();
        if (extension.empty())
            break;

        // Directories
        while (true) {
            auto directory = vpk.reader.read<std::string>();
            if (directory.empty())
                break;

            // Files
            while (true) {
                auto entryname = vpk.reader.read<std::string>();
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

                if (vpk.reader.read<std::uint16_t>() != 0xffff) {
                    // Invalid terminator!
                    return false;
                }

                if (preloadedDataSize > 0) {
                    entry.preloadedData = vpk.reader.readBytes(preloadedDataSize);
                    entry.length += preloadedDataSize;
                }

                vpk.entries[fullDir].push_back(entry);
            }
        }
    }

    // Read VPK2-specific data
    if (vpk.header1.version == 2) {
        // Skip over file data, if any
        vpk.reader.seek(static_cast<long>(vpk.header2.fileDataSectionSize), std::ios_base::cur);

        if (vpk.header2.archiveMD5SectionSize % sizeof(MD5Entry) != 0)
            return false;

        vpk.md5Entries.clear();
        unsigned int entryNum = vpk.header2.archiveMD5SectionSize / sizeof(MD5Entry);
        for (unsigned int i = 0; i < entryNum; i++)
            vpk.md5Entries.push_back(vpk.reader.read<MD5Entry>());

        if (vpk.header2.otherMD5SectionSize != 48)
            return false;

        vpk.treeChecksum = vpk.reader.readBytes<16>();
        vpk.md5EntriesChecksum = vpk.reader.readBytes<16>();
        vpk.wholeFileChecksum = vpk.reader.readBytes<16>();

        if (!vpk.header2.signatureSectionSize)
            return false;

        vpk.publicKey = vpk.reader.readBytes(vpk.reader.read<std::int32_t>());
        vpk.signature = vpk.reader.readBytes(vpk.reader.read<std::int32_t>());
    }

    return true;
}

std::optional<VPKEntry> VPK::findEntry(const std::string& filename_) const {
    auto name = filename_;
    std::replace(name.begin(), name.end(), '\\', '/');
    auto lastSeparator = name.rfind('/');
    auto dir = lastSeparator != std::string::npos ? name.substr(0, lastSeparator) : "";
    name = filename_.substr((lastSeparator + 1));

    if (!this->entries.count(dir)) {
        // There are no files with this extension
        return std::nullopt;
    }

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
        if (entry.filename == name) {
            return entry;
        }
    }

    return std::nullopt;
}

std::optional<std::vector<std::byte>> VPK::readEntry(const VPKEntry& entry) const {
    std::vector<std::byte> output(entry.preloadedData.size() + entry.length, static_cast<std::byte>(0));

    if (!entry.preloadedData.empty()) {
        std::copy(entry.preloadedData.begin(), entry.preloadedData.end(), output.begin());
    }

    if (entry.length == entry.preloadedData.size()) {
        return output;
    }

    if (entry.archiveIndex != 0x7FFF) {
        if (!this->isDirVPK) {
            return std::nullopt;
        }
        char name[1024] {0};
        snprintf(name, sizeof(name) - 1, "%s_%03d.vpk", this->filename.c_str(), entry.archiveIndex);
        InputStream stream{name};
        if (!stream) {
            return std::nullopt;
        }
        stream.seek(static_cast<long>(entry.offset));
        auto bytes = stream.readBytes(entry.length);
        std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<long long>(entry.preloadedData.size()));
    } else {
        InputStream stream{this->filename + ".vpk"};
        if (!stream) {
            return std::nullopt;
        }
        stream.seek(static_cast<long>(entry.offset) + static_cast<long>(this->getHeaderLength()) + static_cast<long>(this->header1.treeSize));
        auto bytes = stream.readBytes(entry.length);
        std::copy(bytes.begin(), bytes.end(), std::back_inserter(output));
    }

    return output;
}
