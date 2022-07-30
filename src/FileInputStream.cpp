#include <vpktool/FileInputStream.h>
#include <tuple>

using namespace vpktool;

FileInputStream::FileInputStream(const std::string& filepath, bool binary) {
    errno = 0;
    this->stream = fopen(filepath.c_str(), binary ? "rb" : "r");
}

FileInputStream::~FileInputStream() {
    fclose(this->stream);
}

FileInputStream::FileInputStream(FileInputStream&& other) noexcept {
    this->stream = other.stream;
}

FileInputStream& FileInputStream::operator=(FileInputStream&& other) noexcept {
    this->stream = other.stream;
    return *this;
}

FileInputStream::operator bool() const {
    return static_cast<bool>(this->stream);
}

bool FileInputStream::operator!() const {
    return !this->operator bool();
}

void FileInputStream::seek(long pos) const {
    this->seek(pos, SEEK_SET);
}

void FileInputStream::seek(long offset, int offsetFrom) const {
    fseek(this->stream, offset, offsetFrom);
}

long FileInputStream::tell() const {
    return ftell(this->stream);
}

std::vector<byte> FileInputStream::readBytes(unsigned int length) const {
    std::vector<byte> out;
    out.resize(length);
    std::ignore = fread(&out[0], sizeof(byte), length, this->stream);
    return out;
}

std::string FileInputStream::readString() const {
    std::string out;
    char temp;
    temp = this->read<char>();
    while (temp != '\0') {
        out += temp;
        temp = this->read<char>();
    }
    return out;
}

byte FileInputStream::peek(long offset) const {
    byte out = '\0';
    this->seek(offset, SEEK_CUR);
    std::ignore = fread(&out, sizeof(byte), 1, this->stream);
    this->seek(-offset - 1, SEEK_CUR);
    return out;
}
