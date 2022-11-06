#pragma once

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace vpktool {

using byte = unsigned char;

/// Changes the endianness of a type. Should only be used with numbers.
template<typename T> inline T swapEndian(T t) {
    union {
        T t;
        byte u8[sizeof(T)];
    } source{}, dest{};
    source.t = t;
    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];
    return dest.t;
}

class InputStream {
public:
    explicit InputStream(const std::string& filepath, bool binary = true);
    InputStream(byte* buffer, std::uint64_t bufferLen);
    ~InputStream();
    InputStream(const InputStream& other) = delete;
    InputStream& operator=(const InputStream& other) = delete;
    InputStream(InputStream&& other) noexcept;
    InputStream& operator=(InputStream&& other) noexcept;
    explicit operator bool() const;
    bool operator!() const;
    void seek(std::uint64_t pos);
    void seek(std::uint64_t offset, int offsetFrom);
    [[nodiscard]] long tell() const;
    [[nodiscard]] std::vector<byte> readBytes(unsigned int length);
    template<typename T>
    T read(bool swapEndian_ = false) {
        T wrongEndian = 0;
        auto bytes = this->readBytes(sizeof(T));
        std::memcpy(&wrongEndian, &bytes[0], sizeof(T));
        return swapEndian_ ? swapEndian<T>(wrongEndian) : wrongEndian;
    }
    [[nodiscard]] std::string readString();
    [[nodiscard]] byte peek(long offset = 0);
protected:
    byte* stream;
    std::uint64_t streamLen;
    std::uint64_t streamPos;
};

} // namespace vpktool
