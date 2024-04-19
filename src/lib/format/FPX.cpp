#include <vpkedit/format/FPX.h>

#include <filesystem>

#include <vpkedit/detail/FileStream.h>

using namespace vpkedit;
using namespace vpkedit::detail;

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

std::unique_ptr<PackFile> FPX::openInternal(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* fpx = new FPX{path, options};
	auto packFile = std::unique_ptr<PackFile>(fpx);

	FileStream reader{fpx->fullFilePath};
	reader.seekInput(0);
	reader.read(fpx->header1);
	if (fpx->header1.signature != FPX_SIGNATURE) {
		// File is not an FPX
		return nullptr;
	}
	if (fpx->header1.version != 10) {
		// Only support v10 FPX files
		return nullptr;
	}
	fpx->options.vpk_version = fpx->header1.version;

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
			if (!fpx->entries.contains(fullDir)) {
				fpx->entries[fullDir] = {};
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

				fpx->entries[fullDir].push_back(entry);

				if (entry.vpk_archiveIndex != VPK_DIR_INDEX && entry.vpk_archiveIndex > fpx->numArchives) {
					fpx->numArchives = entry.vpk_archiveIndex;
				}

				if (callback) {
					callback(fullDir, entry);
				}
			}
		}
	}

	// If there are no archives, -1 will be incremented to 0
	fpx->numArchives++;

	return packFile;
}
