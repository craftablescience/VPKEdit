#include <vpktool/InputStream.h>

using namespace vpktool;

InputStream::InputStream(const std::string& filepath) {
    this->isFile = true;
    this->streamFile.open(filepath, std::ios::in | std::ios::binary);
    this->streamFile.unsetf(std::ios::skipws);
    this->streamBuffer = nullptr;
    this->streamLen = 0;
    this->streamPos = 0;
}

InputStream::InputStream(std::byte* buffer, std::uint64_t bufferLength) {
    this->isFile = false;
    this->streamBuffer = buffer;
    this->streamLen = bufferLength;
    this->streamPos = 0;
}

InputStream::~InputStream() {
    if (this->isFile && this->streamFile.is_open()) {
        this->streamFile.close();
    }
}

InputStream::operator bool() const {
    return static_cast<bool>(this->streamFile);
}

bool InputStream::operator!() const {
    return !this->operator bool();
}

void InputStream::seek(std::uint64_t pos) {
    this->seek(pos, std::ios::beg);
}

void InputStream::seek(std::uint64_t offset, std::ios::seekdir offsetFrom) {
    if (this->isFile) {
        this->streamFile.seekg(static_cast<long>(offset), offsetFrom);
    } else {
        if (offsetFrom == std::ios::beg)
            this->streamPos = offset;
        else if (offsetFrom == std::ios::cur)
            this->streamPos += offsetFrom;
        else if (offsetFrom == std::ios::end)
            this->streamPos = this->streamLen + offsetFrom;
    }
}

std::uint64_t InputStream::tell() {
    if (this->isFile) {
        return this->streamFile.tellg();
    }
    return this->streamPos;
}

std::vector<std::byte> InputStream::readBytes(std::uint64_t length) {
    std::vector<std::byte> out;
    out.resize(length);
    if (this->isFile) {
        this->streamFile.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(length));
    } else {
        for (int i = 0; i < length; i++, this->streamPos++) {
            out.push_back(this->streamBuffer[this->streamPos]);
        }
    }
    return out;
}

std::byte InputStream::peek(long offset) {
    if (this->isFile) {
        return static_cast<std::byte>(this->streamFile.peek());
    }
    return this->streamBuffer[this->streamPos + offset];
}
