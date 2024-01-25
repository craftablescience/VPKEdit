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

bool FileStream::operator!() const {
    return !this->operator bool();
}

void FileStream::seek_input(std::uint64_t offset, std::ios::seekdir offsetFrom) {
    this->streamFile.seekg(static_cast<long>(offset), offsetFrom);
}

std::size_t FileStream::tell_input() {
    return this->streamFile.tellg();
}

void FileStream::seek_output(std::uint64_t offset, std::ios::seekdir offsetFrom) {
    this->streamFile.seekp(static_cast<long>(offset), offsetFrom);
}

std::size_t FileStream::tell_output() {
    return this->streamFile.tellp();
}

std::byte FileStream::peek(long offset) {
	return static_cast<std::byte>(this->streamFile.peek());
}

std::vector<std::byte> FileStream::read_bytes(std::uint64_t length) {
    std::vector<std::byte> out;
    out.resize(length);
    this->streamFile.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(length));
    return out;
}

std::string FileStream::read_string() {
	std::string out;
	this->read(out);
	return out;
}

std::string FileStream::read_string(std::size_t n, bool stopOnNullTerminator) {
	std::string out;
	this->read(out, n, stopOnNullTerminator);
	return out;
}

void FileStream::write_bytes(const std::vector<std::byte>& buffer) {
    this->streamFile.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
}

void FileStream::flush() {
    this->streamFile.flush();
}
