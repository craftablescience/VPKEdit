#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

namespace vpkedit::detail {

template<typename T>
concept PODType = std::is_trivial_v<T> && std::is_standard_layout_v<T>;

enum FileStreamOptions {
    FILESTREAM_OPT_READ                  = 1 << 0,
    FILESTREAM_OPT_WRITE                 = 1 << 1,
    FILESTREAM_OPT_APPEND                = 1 << 2,
    FILESTREAM_OPT_TRUNCATE              = 1 << 3,
    FILESTREAM_OPT_CREATE_IF_NONEXISTENT = 1 << 4,
};

class FileStream {
public:
    explicit FileStream(const std::string& filepath, int options = FILESTREAM_OPT_READ);
    FileStream(const FileStream& other) = delete;
    FileStream& operator=(const FileStream& other) = delete;
    FileStream(FileStream&& other) noexcept = default;
    FileStream& operator=(FileStream&& other) noexcept = default;

    explicit operator bool() const;
    bool operator!() const;

	void seek_input(std::size_t offset, std::ios::seekdir offsetFrom = std::ios::beg);

	template<PODType T = std::byte>
	void skip_input(std::size_t n = 1) {
		if (!n) {
			return;
		}
		this->seek_input(sizeof(T) * n, std::ios::cur);
	}

	[[nodiscard]] std::size_t tell_input();

	void seek_output(std::size_t offset, std::ios::seekdir offsetFrom = std::ios::beg);

	template<PODType T = std::byte>
	void skip_output(std::size_t n = 1) {
		if (!n) {
			return;
		}
		this->seek_output(sizeof(T) * n, std::ios::cur);
	}

	[[nodiscard]] std::size_t tell_output();

	[[nodiscard]] std::byte peek(long offset = 0);

	template<PODType T>
	[[nodiscard]] T read() {
		T obj{};
		this->read(obj);
		return obj;
	}

	template<std::size_t L>
	[[nodiscard]] std::array<std::byte, L> read_bytes() {
		std::array<std::byte, L> out;
		this->streamFile.read(reinterpret_cast<char*>(out.data()), L);
		return out;
	}

	[[nodiscard]] std::vector<std::byte> read_bytes(std::size_t length);

	[[nodiscard]] std::string read_string();

	[[nodiscard]] std::string read_string(std::size_t n, bool stopOnNullTerminator = true);

	template<PODType T>
	void read(T& obj) {
		this->streamFile.read(reinterpret_cast<char*>(&obj), sizeof(T));
	}

	template<PODType T, std::size_t N>
	void read(T(&obj)[N]) {
		for (int i = 0; i < N; i++) {
			obj[i] = this->read<T>();
		}
	}

	template<PODType T, std::size_t N>
	void read(std::array<T, N>& obj) {
		for (int i = 0; i < N; i++) {
			obj[i] = this->read<T>();
		}
	}

	template<PODType T>
	void read(std::vector<T>& obj, std::size_t n) {
		obj.clear();
		if (!n) {
			return;
		}
		obj.reserve(n);
		for (int i = 0; i < n; i++) {
			obj.push_back(this->read<T>());
		}
	}

	void read(std::string& obj) {
		obj.clear();
		char temp = this->read<char>();
		while (temp != '\0') {
			obj += temp;
			temp = this->read<char>();
		}
	}

	void read(std::string& obj, std::size_t n, bool stopOnNullTerminator = true) {
		obj.clear();
		if (!n) {
			return;
		}
		obj.reserve(n);
		for (int i = 0; i < n; i++) {
			char temp = this->read<char>();
			if (temp == '\0' && stopOnNullTerminator) {
				// Read the required number of characters and exit
				this->skip_input<char>(n - i - 1);
				break;
			}
			obj += temp;
		}
	}

	template<PODType T>
	void write(T obj) {
		this->streamFile.write(reinterpret_cast<const char*>(&obj), sizeof(T));
	}

	template<std::size_t L>
	void write_bytes(const std::array<std::byte, L>& buffer) {
		this->streamFile.write(reinterpret_cast<const char*>(buffer.data()), L);
	}

	void write_bytes(const std::vector<std::byte>& buffer);

	template<PODType T, std::size_t N>
	void write(T(&obj)[N]) {
		for (int i = 0; i < N; i++) {
			this->write(obj[i]);
		}
	}

	template<PODType T, std::size_t N>
	void write(std::array<T, N>& obj) {
		for (int i = 0; i < N; i++) {
			this->write(obj[i]);
		}
	}

	template<PODType T>
	void write(const std::vector<T>& obj) {
		for (int i = 0; i < obj.size(); i++) {
			this->write(obj[i]);
		}
	}

	template<PODType T>
	void write_unsafe(T* obj, std::size_t len) {
		this->streamFile.write(reinterpret_cast<const char*>(obj), sizeof(T) * len);
	}

	void write(std::string_view obj) {
		this->streamFile.write(obj.data(), static_cast<std::streamsize>(obj.size()));
	}

	void flush();

protected:
    std::fstream streamFile;
};

} // namespace vpkedit::detail
