#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

namespace vpkedit::detail {

enum FileStreamOptions {
    FILESTREAM_OPT_READ                  = 1 << 0,
    FILESTREAM_OPT_WRITE                 = 1 << 1,
    FILESTREAM_OPT_TRUNCATE              = 1 << 2,
    FILESTREAM_OPT_CREATE_IF_NONEXISTENT = 1 << 3,
};

class FileStream {
public:
    explicit FileStream(const std::string& filepath, int options = FILESTREAM_OPT_READ);
    FileStream(std::byte* buffer, std::uint64_t bufferLength);
    ~FileStream();
    FileStream(const FileStream& other) = delete;
    FileStream& operator=(const FileStream& other) = delete;
    FileStream(FileStream&& other) noexcept = default;
    FileStream& operator=(FileStream&& other) noexcept = default;

    explicit operator bool() const;
    bool operator!() const;

    void seekInput(std::uint64_t offset, std::ios::seekdir offsetFrom = std::ios::beg);

    [[nodiscard]] std::uint64_t tellInput();

    void seekOutput(std::uint64_t offset, std::ios::seekdir offsetFrom = std::ios::beg);

    [[nodiscard]] std::uint64_t tellOutput();

    template<std::uint64_t L>
    [[nodiscard]] std::array<std::byte, L> readBytes() {
        std::array<std::byte, L> out;
        if (this->isFile) {
            this->streamFile.read(reinterpret_cast<char*>(out.data()), L);
        } else {
            for (int i = 0; i < L; i++, this->streamPosRead++) {
                out[i] = this->streamBuffer[this->streamPosRead];
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

    template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
    void read(T& obj) {
        if (this->isFile) {
            this->streamFile.read(reinterpret_cast<char*>(&obj), sizeof(T));
        } else {
            for (int i = 0; i < sizeof(T); i++, this->streamPosRead++) {
                reinterpret_cast<std::byte*>(&obj)[i] = this->streamBuffer[this->streamPosRead];
            }
        }
    }

    void read(std::string& obj) {
        char temp = this->read<char>();
        while (temp != '\0') {
            obj += temp;
            temp = this->read<char>();
        }
    }

    [[nodiscard]] std::byte peek(long offset = 0);

    template<std::uint64_t L>
    void writeBytes(const std::array<std::byte, L> obj) {
        if (this->isFile) {
            this->streamFile.write(reinterpret_cast<const char*>(obj.data()), L);
        } else {
            return; // unimplemented for buffers
        }
    }

    void writeBytes(const std::vector<std::byte>& buffer);

    template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
    void write(T obj) {
        this->write(&obj);
    }

    template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
    void write(T* obj) {
        if (this->isFile) {
            this->streamFile.write(reinterpret_cast<const char*>(obj), sizeof(T));
        } else {
            return; // unimplemented for buffers
        }
    }

    template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
    void write(T* obj, std::size_t len) {
        if (this->isFile) {
            this->streamFile.write(reinterpret_cast<const char*>(obj), sizeof(T) * len);
        } else {
            return; // unimplemented for buffers
        }
    }

    void write(const std::string& obj) {
        if (this->isFile) {
            this->streamFile.write(obj.data(), static_cast<std::streamsize>(obj.size()));
        } else {
            return; // unimplemented for buffers
        }
    }

    void flush();

protected:
    std::fstream streamFile;

    std::byte* streamBuffer;
    std::uint64_t streamLen;
    std::uint64_t streamPosRead;
    std::uint64_t streamPosWrite;

    bool isFile;
};

} // namespace vpkedit::detail
