#include <vpkedit/BSP.h>

#include <cstring>
#include <filesystem>

#include <MD5.h>
#include <mz.h>
#include <mz_strm.h>
#include <mz_strm_os.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>
#include <vpkedit/detail/CRC.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

const std::string BSP::BSP_TEMP_ZIP_PATH = (std::filesystem::temp_directory_path() / "tmp_bsp_paklump.zip").string();

BSP::BSP(const std::string& fullFilePath_, PackFileOptions options_)
		: ZIP(fullFilePath_, options_)
		, reader(fullFilePath_) {
	this->type = PackFileType::BSP;
}

std::unique_ptr<PackFile> BSP::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* bsp = new BSP{path, options};
	auto packFile = std::unique_ptr<PackFile>(bsp);

	bsp->reader.seekInput(0);
	bsp->reader.read(bsp->header.signature);
	if (bsp->header.signature != BSP_ID) {
		// File is not a BSP
		return nullptr;
	}
	bsp->reader.read(bsp->header.version);
	bsp->reader.read(bsp->header.lumps);
	bsp->reader.read(bsp->header.mapRevision);

	if (bsp->header.lumps[BSP_LUMP_PAKFILE_INDEX].offset == 0 || bsp->header.lumps[BSP_LUMP_PAKFILE_INDEX].length == 0) {
		// No paklump, create an empty zip
		void* writeStreamHandle = mz_stream_os_create();
		if (mz_stream_os_open(writeStreamHandle, BSP::BSP_TEMP_ZIP_PATH.c_str(), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE)) {
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
		bsp->reader.seekInput(bsp->header.lumps[BSP_LUMP_PAKFILE_INDEX].offset);
		auto binData = bsp->reader.readBytes(bsp->header.lumps[BSP_LUMP_PAKFILE_INDEX].length);

		FileStream writer{BSP::BSP_TEMP_ZIP_PATH, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
		writer.writeBytes(binData);
	}

	if (!bsp->openZIP(BSP::BSP_TEMP_ZIP_PATH)) {
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
		if (!options.allowUppercaseLettersInFilenames) {
			::toLowerCase(entry.path);
		}

		entry.length = fileInfo->uncompressed_size;
		entry.compressedLength = fileInfo->compressed_size;
		entry.crc32 = fileInfo->crc;
		entry.zip_compressionMethod = fileInfo->compression_method;

		auto parentDir = std::filesystem::path(entry.path).parent_path().string();
		::normalizeSlashes(parentDir);
		if (!options.allowUppercaseLettersInFilenames) {
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

	// Write the pakfile lump
	{
		auto binData = ::readFileData(ZIP::TEMP_ZIP_PATH);
		this->moveLumpToWritableSpace(BSP_LUMP_PAKFILE_INDEX, static_cast<int>(binData.size()));

		FileStream writer{this->fullFilePath, FILESTREAM_OPT_WRITE};
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

	// Close our ZIP and reopen it
	this->closeZIP();
	std::filesystem::rename(ZIP::TEMP_ZIP_PATH, BSP::BSP_TEMP_ZIP_PATH);
	if (!this->openZIP(BSP::BSP_TEMP_ZIP_PATH)) {
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
	if (!lumpsAfterPaklumpIndices.empty()) {
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
}
