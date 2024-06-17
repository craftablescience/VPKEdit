#include <vpkedit/detail/FileStream.h>

#include <filesystem>

using namespace vpkedit::detail;

FileStream::FileStream(const std::string& filepath, int options) {
	if ((options & FILESTREAM_OPT_CREATE_IF_NONEXISTENT) && !std::filesystem::exists(filepath)) {
		if (!std::filesystem::exists(std::filesystem::path{filepath}.parent_path())) {
			std::error_code ec;
			std::filesystem::create_directories(std::filesystem::path{filepath}.parent_path(), ec);
			ec.clear();
		}
		std::ofstream create(filepath, std::ios::trunc);
	}
	std::ios::openmode openMode = std::ios::binary;
	if (options & FILESTREAM_OPT_READ) {
		openMode |= std::ios::in;
	}
	if (options & FILESTREAM_OPT_WRITE) {
		openMode |= std::ios::out;
	}
	if (options & FILESTREAM_OPT_APPEND) {
		openMode |= std::ios::out;
		openMode |= std::ios::app;
	}
	if (options & FILESTREAM_OPT_TRUNCATE) {
		openMode |= std::ios::out;
		openMode |= std::ios::trunc;
	}
	this->streamFile.open(filepath, openMode);
	this->streamFile.unsetf(std::ios::skipws);
}

FileStream::operator bool() const {
	return static_cast<bool>(this->streamFile);
}

void FileStream::seekInput(std::size_t offset, std::ios::seekdir offsetFrom) {
	this->streamFile.seekg(static_cast<std::streamsize>(offset), offsetFrom);
}

void FileStream::seekOutput(std::size_t offset, std::ios::seekdir offsetFrom) {
	this->streamFile.seekp(static_cast<std::streamsize>(offset), offsetFrom);
}

std::size_t FileStream::tellInput() {
	return this->streamFile.tellg();
}

std::size_t FileStream::tellOutput() {
	return this->streamFile.tellp();
}

std::vector<std::byte> FileStream::readBytes(std::size_t length) {
	std::vector<std::byte> out;
	out.resize(length);
	this->streamFile.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(length));
	return out;
}

std::string FileStream::readString() {
	std::string out;
	this->read(out);
	return out;
}

std::string FileStream::readString(std::size_t n, bool stopOnNullTerminator) {
	std::string out;
	this->read(out, n, stopOnNullTerminator);
	return out;
}

void FileStream::writeBytes(const std::vector<std::byte>& buffer) {
	this->streamFile.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
}

void FileStream::flush() {
	this->streamFile.flush();
}
