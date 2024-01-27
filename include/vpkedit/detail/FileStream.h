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
	FILESTREAM_OPT_APPEND                = 1 << 2,
	FILESTREAM_OPT_TRUNCATE              = 1 << 3,
	FILESTREAM_OPT_CREATE_IF_NONEXISTENT = 1 << 4,
};

template<typename T>
concept PODType = std::is_trivial_v<T> && std::is_standard_layout_v<T>;

class FileStream {
public:
	explicit FileStream(const std::string& filepath, int options = FILESTREAM_OPT_READ);
	FileStream(const FileStream& other) = delete;
	FileStream& operator=(const FileStream& other) = delete;
	FileStream(FileStream&& other) noexcept = default;
	FileStream& operator=(FileStream&& other) noexcept = default;

	explicit operator bool() const;

	void seekInput(std::uint64_t offset, std::ios::seekdir offsetFrom = std::ios::beg);

	[[nodiscard]] std::uint64_t tellInput();

	void seekOutput(std::uint64_t offset, std::ios::seekdir offsetFrom = std::ios::beg);

	[[nodiscard]] std::uint64_t tellOutput();

	template<std::uint64_t L>
	[[nodiscard]] std::array<std::byte, L> readBytes() {
		std::array<std::byte, L> out;
		this->streamFile.read(reinterpret_cast<char*>(out.data()), L);
		return out;
	}

	[[nodiscard]] std::vector<std::byte> readBytes(std::uint64_t length);

	template<PODType T>
	[[nodiscard]] T read() {
		T obj{};
		this->read(obj);
		return obj;
	}

	template<PODType T>
	void read(T& obj) {
		this->streamFile.read(reinterpret_cast<char*>(&obj), sizeof(T));
	}

	void read(std::string& obj) {
		char temp = this->read<char>();
		while (temp != '\0') {
			obj += temp;
			temp = this->read<char>();
		}
	}

	template<std::uint64_t L>
	void writeBytes(const std::array<std::byte, L> obj) {
		this->streamFile.write(reinterpret_cast<const char*>(obj.data()), L);
	}

	void writeBytes(const std::vector<std::byte>& buffer);

	template<PODType T>
	void write(T obj) {
		this->write(&obj);
	}

	template<PODType T>
	void write(T* obj) {
		this->streamFile.write(reinterpret_cast<const char*>(obj), sizeof(T));
	}

	template<PODType T>
	void write(T* obj, std::size_t len) {
		this->streamFile.write(reinterpret_cast<const char*>(obj), sizeof(T) * len);
	}

	void write(const std::string& obj) {
		this->streamFile.write(obj.data(), static_cast<std::streamsize>(obj.size()));
	}

	void flush();

protected:
	std::fstream streamFile;
};

} // namespace vpkedit::detail
