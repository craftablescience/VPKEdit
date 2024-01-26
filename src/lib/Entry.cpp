#include <vpkedit/Entry.h>

#include <filesystem>

using namespace vpkedit;

std::string Entry::getParentPath() const {
	return std::filesystem::path{this->path}.parent_path().string();
}

std::string Entry::getFilename() const {
	return std::filesystem::path{this->path}.filename().string();
}

std::string Entry::getStem() const {
	return std::filesystem::path{this->path}.stem().string();
}

std::string Entry::getExtension() const {
	auto ext = std::filesystem::path{this->path}.extension().string();
	if (!ext.empty() && ext.at(0) == '.') {
		ext = ext.substr(1);
	}
	return ext;
}
