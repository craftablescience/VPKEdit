#include <vpkedit/detail/FileStream.h>

#include <filesystem>

using namespace vpkedit::detail;

FileStream::FileStream(const std::string& filepath, int options) {
	if ((options & FILESTREAM_OPT_CREATE_IF_NONEXISTENT) && !std::filesystem::exists(filepath)) {
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
		openMode |= std::ios::app;
	}
	if (options & FILESTREAM_OPT_TRUNCATE) {
		openMode |= std::ios::trunc;
	}
	this->streamFile.open(filepath, openMode);
	this->streamFile.unsetf(std::ios::skipws);
}

FileStream::operator bool() const {
	return static_cast<bool>(this->streamFile);
}

void FileStream::seekInput(std::uint64_t offset, std::ios::seekdir offsetFrom) {
	this->streamFile.seekg(static_cast<long>(offset), offsetFrom);
}

std::uint64_t FileStream::tellInput() {
	return this->streamFile.tellg();
}

void FileStream::seekOutput(std::uint64_t offset, std::ios::seekdir offsetFrom) {
	this->streamFile.seekp(static_cast<long>(offset), offsetFrom);
}

std::uint64_t FileStream::tellOutput() {
	return this->streamFile.tellp();
}

std::vector<std::byte> FileStream::readBytes(std::uint64_t length) {
	std::vector<std::byte> out;
	out.resize(length);
	this->streamFile.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(length));
	return out;
}

void FileStream::writeBytes(const std::vector<std::byte>& buffer) {
	this->streamFile.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
}

void FileStream::flush() {
	this->streamFile.flush();
}
