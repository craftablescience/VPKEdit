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

	void seekInput(std::size_t offset, std::ios::seekdir offsetFrom = std::ios::beg);

	void seekOutput(std::size_t offset, std::ios::seekdir offsetFrom = std::ios::beg);

	template<PODType T = std::byte>
	void skipInput(std::size_t n = 1) {
		if (!n) {
			return;
		}
		this->seekInput(sizeof(T) * n, std::ios::cur);
	}

	template<PODType T = std::byte>
	void skipOutput(std::size_t n = 1) {
		if (!n) {
			return;
		}
		this->seekOutput(sizeof(T) * n, std::ios::cur);
	}

	[[nodiscard]] std::size_t tellInput();

	[[nodiscard]] std::size_t tellOutput();

	template<PODType T>
	[[nodiscard]] T read() {
		T obj{};
		this->read(obj);
		return obj;
	}

	template<std::size_t L>
	[[nodiscard]] std::array<std::byte, L> readBytes() {
		std::array<std::byte, L> out;
		this->streamFile.read(reinterpret_cast<char*>(out.data()), L);
		return out;
	}

	[[nodiscard]] std::vector<std::byte> readBytes(std::size_t length);

	[[nodiscard]] std::string readString();

	[[nodiscard]] std::string readString(std::size_t n, bool stopOnNullTerminator = true);

	template<PODType T>
	void read(T& obj) {
		this->streamFile.read(reinterpret_cast<char*>(&obj), sizeof(T));
	}

	template<PODType T, std::size_t N>
	void read(T(&obj)[N]) {
		this->streamFile.read(reinterpret_cast<char*>(&obj[0]), sizeof(T) * N);
	}

	template<typename T, std::size_t N>
	void read(std::array<T, N>& obj) {
		for (int i = 0; i < N; i++) {
			obj[i] = this->read<T>();
		}
	}

	template<typename T>
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
				this->skipInput<char>(n - i - 1);
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
	void writeBytes(const std::array<std::byte, L> obj) {
		this->streamFile.write(reinterpret_cast<const char*>(obj.data()), L);
	}

	void writeBytes(const std::vector<std::byte>& buffer);

	template<PODType T, std::size_t N>
	void write(T(&obj)[N]) {
		this->streamFile.write(reinterpret_cast<const char*>(&obj[0]), sizeof(T) * N);
	}

	template<typename T, std::size_t N>
	void write(const std::array<T, N>& obj) {
		for (int i = 0; i < N; i++) {
			this->write(obj[i]);
		}
	}

	template<typename T>
	void write(const std::vector<T>& obj) {
		for (T item : obj) {
			this->write(item);
		}
	}

	void write(const std::string& obj, bool includeTerminator = true) {
		this->streamFile.write(obj.data(), static_cast<std::streamsize>(obj.size()));
		if (includeTerminator) {
			this->write('\0');
		}
	}

	void write(const std::string& obj, std::size_t n) {
		for (int i = 0; i < n; i++) {
			if (i < obj.size()) {
				this->write(obj[i]);
			} else {
				this->write('\0');
			}
		}
	}

	void flush();

protected:
	std::fstream streamFile;
};

} // namespace vpkedit::detail
