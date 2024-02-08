#include <vpkedit/detail/Misc.h>

#include <algorithm>
#include <filesystem>
#include <cctype>

#include <vpkedit/detail/FileStream.h>

using namespace vpkedit;

void detail::toLowerCase(std::string& input) {
	std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c){ return std::tolower(c); });
}

void detail::normalizeSlashes(std::string& path) {
	std::replace(path.begin(), path.end(), '\\', '/');
	if (path.starts_with('/')) {
		path = path.substr(1);
	}
	if (path.ends_with('/')) {
		path.pop_back();
	}
}

std::pair<std::string, std::string> detail::splitFilenameAndParentDir(const std::string& filename) {
	auto name = filename;
	normalizeSlashes(name);

	auto lastSeparator = name.rfind('/');
	auto dir = lastSeparator != std::string::npos ? name.substr(0, lastSeparator) : "";
	name = filename.substr(lastSeparator + 1);

	return {dir, name};
}

std::vector<std::byte> detail::readFileData(const std::string& filepath, std::size_t preloadBytesOffset) {
	FileStream stream{filepath};
	if (!stream) {
		return {};
	}
	stream.seekInput(preloadBytesOffset);
	return stream.readBytes(std::filesystem::file_size(filepath) - preloadBytesOffset);
}
