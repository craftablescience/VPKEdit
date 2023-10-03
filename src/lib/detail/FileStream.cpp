#include <vpkedit/detail/FileStream.h>

#include <filesystem>

using namespace vpkedit::detail;

FileStream::FileStream(const std::string& filepath, int options) {
    this->isFile = true;
    if ((options & FILESTREAM_OPT_CREATE_IF_NONEXISTENT) && !std::filesystem::exists(filepath)) {
        std::ofstream create(filepath, std::ios::trunc);
    }
    auto openMode = std::ios::binary;
    if (options & FILESTREAM_OPT_READ) {
        openMode |= std::ios::in;
    }
    if (options & FILESTREAM_OPT_WRITE) {
        openMode |= std::ios::out;
    }
    if (options & FILESTREAM_OPT_TRUNCATE) {
        openMode |= std::ios::trunc;
    }
    this->streamFile.open(filepath, openMode);
    this->streamFile.unsetf(std::ios::skipws);

    this->streamBuffer = nullptr;
    this->streamLen = 0;
    this->streamPosRead = 0;
    this->streamPosWrite = 0;
}

FileStream::FileStream(std::byte* buffer, std::uint64_t bufferLength) {
    this->isFile = false;
    this->streamBuffer = buffer;
    this->streamLen = bufferLength;
    this->streamPosRead = 0;
    this->streamPosWrite = 0;
}

FileStream::~FileStream() {
    if (this->isFile && this->streamFile.is_open()) {
        this->streamFile.close();
    }
}

FileStream::operator bool() const {
    return static_cast<bool>(this->streamFile);
}

bool FileStream::operator!() const {
    return !this->operator bool();
}

void FileStream::seekInput(std::uint64_t offset, std::ios::seekdir offsetFrom) {
    if (this->isFile) {
        this->streamFile.seekg(static_cast<long>(offset), offsetFrom);
    } else {
        if (offsetFrom == std::ios::beg)
            this->streamPosRead = offset;
        else if (offsetFrom == std::ios::cur)
            this->streamPosRead += offsetFrom;
        else if (offsetFrom == std::ios::end)
            this->streamPosRead = this->streamLen + offsetFrom;
    }
}

std::uint64_t FileStream::tellInput() {
    if (this->isFile) {
        return this->streamFile.tellg();
    }
    return this->streamPosRead;
}

void FileStream::seekOutput(std::uint64_t offset, std::ios::seekdir offsetFrom) {
    if (this->isFile) {
        this->streamFile.seekp(static_cast<long>(offset), offsetFrom);
    } else {
        if (offsetFrom == std::ios::beg)
            this->streamPosWrite = offset;
        else if (offsetFrom == std::ios::cur)
            this->streamPosWrite += offsetFrom;
        else if (offsetFrom == std::ios::end)
            this->streamPosWrite = this->streamLen + offsetFrom;
    }
}

std::uint64_t FileStream::tellOutput() {
    if (this->isFile) {
        return this->streamFile.tellp();
    }
    return this->streamPosRead;
}

std::vector<std::byte> FileStream::readBytes(std::uint64_t length) {
    std::vector<std::byte> out;
    out.resize(length);
    if (this->isFile) {
        this->streamFile.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(length));
    } else {
        for (int i = 0; i < length; i++, this->streamPosRead++) {
            out.push_back(this->streamBuffer[this->streamPosRead]);
        }
    }
    return out;
}

std::byte FileStream::peek(long offset) {
    if (this->isFile) {
        return static_cast<std::byte>(this->streamFile.peek());
    }
    return this->streamBuffer[this->streamPosRead + offset];
}

void FileStream::writeBytes(const std::vector<std::byte>& buffer) {
    if (this->isFile) {
        this->streamFile.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    } else {
        return; // unimplemented for buffers
    }
}

void FileStream::flush() {
    if (this->isFile) {
        this->streamFile.flush();
    }
    // Do nothing if buffer, no point
}
