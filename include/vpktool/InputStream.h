#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

namespace vpktool {

class InputStream {
public:
    explicit InputStream(const std::string& filepath);
    InputStream(std::byte* buffer, std::uint64_t bufferLength);
    ~InputStream();
    InputStream(const InputStream& other) = delete;
    InputStream& operator=(const InputStream& other) = delete;
    InputStream(InputStream&& other) noexcept = default;
    InputStream& operator=(InputStream&& other) noexcept = default;

    explicit operator bool() const;
    bool operator!() const;

    void seek(std::uint64_t pos);
    void seek(std::uint64_t offset, std::ios::seekdir offsetFrom);

    [[nodiscard]] std::uint64_t tell();

    template<std::uint64_t L>
    [[nodiscard]] std::array<std::byte, L> readBytes() {
        std::array<std::byte, L> out;
        if (this->isFile) {
            this->streamFile.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(L));
        } else {
            for (int i = 0; i < L; i++, this->streamPos++) {
                out[i] = this->streamBuffer[this->streamPos];
            }
        }
        return out;
    }

    [[nodiscard]] std::vector<std::byte> readBytes(std::uint64_t length);

    template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T> && std::is_trivially_constructible_v<T>, bool> = true>
    [[nodiscard]] T read() {
        T obj{};
        this->read(obj);
        return obj;
    }

    template<typename T>
    void read(T& obj) {
        if (this->isFile) {
            this->streamFile.read(reinterpret_cast<char*>(&obj), static_cast<std::streamsize>(sizeof(T)));
        } else {
            for (int i = 0; i < sizeof(T); i++, this->streamPos++) {
                reinterpret_cast<std::byte*>(&obj)[i] = this->streamBuffer[this->streamPos];
            }
        }
    }

    void read(std::string& obj) {
        char temp;
        temp = this->read<char>();
        while (temp != '\0') {
            obj += temp;
            temp = this->read<char>();
        }
    }

    [[nodiscard]] std::byte peek(long offset = 0);

protected:
    std::ifstream streamFile;

    std::byte* streamBuffer;
    std::uint64_t streamLen;
    std::uint64_t streamPos;

    bool isFile;
};

} // namespace vpktool
