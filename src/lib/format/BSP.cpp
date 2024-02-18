#include <vpkedit/format/BSP.h>

#include <filesystem>

#include <mz.h>
#include <mz_strm.h>
#include <mz_strm_os.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

const std::string BSP::TEMP_ZIP_PATH = (std::filesystem::temp_directory_path() / "tmp_bsp_paklump.zip").string();

BSP::BSP(const std::string& fullFilePath_, PackFileOptions options_)
		: ZIP(fullFilePath_, options_) {
	this->type = PackFileType::BSP;
}

std::unique_ptr<PackFile> BSP::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* bsp = new BSP{path, options};
	auto packFile = std::unique_ptr<PackFile>(bsp);

	FileStream reader{bsp->fullFilePath};
	reader.seekInput(0);

	reader.read(bsp->header.signature);
	if (bsp->header.signature != BSP_SIGNATURE) {
		// File is not a BSP
		return nullptr;
	}
	reader.read(bsp->header.version);
	reader.read(bsp->header.lumps);
	reader.read(bsp->header.mapRevision);

	if (bsp->header.lumps[BSP_LUMP_PAKFILE_INDEX].offset == 0 || bsp->header.lumps[BSP_LUMP_PAKFILE_INDEX].length == 0) {
		// No paklump, create an empty zip
		void* writeStreamHandle = mz_stream_os_create();
		if (mz_stream_os_open(writeStreamHandle, BSP::TEMP_ZIP_PATH.c_str(), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE)) {
			return nullptr;
		}
		void* writeZipHandle = mz_zip_writer_create();
		if (mz_zip_writer_open(writeZipHandle, writeStreamHandle, 0)) {
			return nullptr;
		}
		if (mz_zip_writer_close(writeZipHandle)) {
			return nullptr;
		}
		mz_zip_writer_delete(&writeZipHandle);
		if (mz_stream_os_close(writeStreamHandle)) {
			return nullptr;
		}
		mz_stream_os_delete(&writeStreamHandle);
	} else {
		// Extract the paklump to a temp dir
		reader.seekInput(bsp->header.lumps[BSP_LUMP_PAKFILE_INDEX].offset);
		auto binData = reader.readBytes(bsp->header.lumps[BSP_LUMP_PAKFILE_INDEX].length);

		FileStream writer{BSP::TEMP_ZIP_PATH, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
		writer.writeBytes(binData);
	}

	if (!bsp->openZIP(BSP::TEMP_ZIP_PATH)) {
		return nullptr;
	}

	for (auto code = mz_zip_goto_first_entry(bsp->zipHandle); code == MZ_OK; code = mz_zip_goto_next_entry(bsp->zipHandle)) {
		mz_zip_file* fileInfo = nullptr;
		if (mz_zip_entry_get_info(bsp->zipHandle, &fileInfo)) {
			return nullptr;
		}
		if (mz_zip_entry_is_dir(bsp->zipHandle) == MZ_OK) {
			continue;
		}

		Entry entry = createNewEntry();
		entry.path = fileInfo->filename;
		::normalizeSlashes(entry.path);
		if (!bsp->isCaseSensitive()) {
			::toLowerCase(entry.path);
		}

		entry.flags = fileInfo->compression_method;
		entry.length = fileInfo->uncompressed_size;
		entry.compressedLength = fileInfo->compressed_size;
		entry.crc32 = fileInfo->crc;

		auto parentDir = std::filesystem::path(entry.path).parent_path().string();
		::normalizeSlashes(parentDir);
		if (!bsp->isCaseSensitive()) {
			::toLowerCase(parentDir);
		}

		if (!bsp->entries.contains(parentDir)) {
			bsp->entries[parentDir] = {};
		}
		bsp->entries[parentDir].push_back(entry);

		if (callback) {
			callback(parentDir, entry);
		}
	}

	return packFile;
}

bool BSP::bake(const std::string& outputDir_, const Callback& callback) {
	// Get the proper file output folder
	std::string outputDir = this->getBakeOutputDir(outputDir_);
	std::string outputPath = outputDir + '/' + this->getFilename();

	// Use temp folder so we can read from the current ZIP
	if (!this->bakeTempZip(ZIP::TEMP_ZIP_PATH, callback)) {
		return false;
	}
	this->mergeUnbakedEntries();

	// Close the ZIP
	this->closeZIP();

	// Write the pakfile lump
	{
		auto binData = ::readFileData(ZIP::TEMP_ZIP_PATH);
		this->moveLumpToWritableSpace(BSP_LUMP_PAKFILE_INDEX, static_cast<int>(binData.size()));

		FileStream writer{this->fullFilePath, FILESTREAM_OPT_READ | FILESTREAM_OPT_WRITE};
		writer.seekOutput(0);

		writer.write(this->header.signature);
		writer.write(this->header.version);
		writer.write(this->header.lumps);
		writer.write(this->header.mapRevision);
		writer.seekOutput(this->header.lumps[BSP_LUMP_PAKFILE_INDEX].offset);
		writer.writeBytes(binData);
	}

	// If the output path is different, copy the entire BSP there
	if (outputPath != this->fullFilePath) {
		std::filesystem::copy_file(this->fullFilePath, outputPath, std::filesystem::copy_options::overwrite_existing);
	}

	// Rename and reopen the ZIP
	std::filesystem::rename(ZIP::TEMP_ZIP_PATH, BSP::TEMP_ZIP_PATH);
	if (!this->openZIP(BSP::TEMP_ZIP_PATH)) {
		return false;
	}
	PackFile::setFullFilePath(outputDir);
	return true;
}

void BSP::moveLumpToWritableSpace(int lumpToMove, int newSize) {
	this->header.lumps[lumpToMove].length = newSize;

	// If the zip is at the end of the file we just overwrite it, otherwise we have to shift some lumps over
	std::vector<int> lumpsAfterPaklumpIndices;
	for (int i = 0; i < this->header.lumps.size(); i++) {
		if (this->header.lumps[i].offset > this->header.lumps[lumpToMove].offset) {
			lumpsAfterPaklumpIndices.push_back(i);
		}
	}
	if (lumpsAfterPaklumpIndices.empty()) {
		return;
	}

	// Get the exact area to move
	int moveOffsetStart = INT_MAX, moveOffsetEnd = 0;
	for (int lumpIndex : lumpsAfterPaklumpIndices) {
		if (this->header.lumps[lumpIndex].offset < moveOffsetStart) {
			moveOffsetStart = this->header.lumps[lumpIndex].offset;
		}
		if (auto offsetAndLength = this->header.lumps[lumpIndex].offset + this->header.lumps[lumpIndex].length; offsetAndLength > moveOffsetEnd) {
			moveOffsetEnd = offsetAndLength;
		}
	}

	// Get where to move it
	int lastLumpBeforePaklumpOffset = 0, lastLumpBeforePaklumpLength = 0;
	for (const Lump& lump : this->header.lumps) {
		if (lump.offset < this->header.lumps[lumpToMove].offset && lump.offset > lastLumpBeforePaklumpOffset) {
			lastLumpBeforePaklumpOffset = lump.offset;
			lastLumpBeforePaklumpLength = lump.length;
		}
	}

	// Move all the lumps after paklump back
	FileStream bsp{this->fullFilePath, FILESTREAM_OPT_READ | FILESTREAM_OPT_WRITE};
	bsp.seekInput(moveOffsetStart);
	auto lumpsData = bsp.readBytes(moveOffsetEnd - moveOffsetStart);
	bsp.seekOutput(lastLumpBeforePaklumpOffset + lastLumpBeforePaklumpLength);
	bsp.writeBytes(lumpsData);

	// Fix the offsets
	for (int lumpIndex : lumpsAfterPaklumpIndices) {
		this->header.lumps[lumpIndex].offset -= newSize;
	}
	this->header.lumps[lumpToMove].offset = lastLumpBeforePaklumpOffset + lastLumpBeforePaklumpLength + static_cast<int>(lumpsData.size());
}

BSP::operator std::string() const {
	return PackFile::operator std::string() +
		" | Version v" + std::to_string(this->header.version) +
		" | Map Revision " + std::to_string(this->header.mapRevision);
}
