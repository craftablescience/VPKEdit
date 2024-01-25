#include <vpkedit/Entry.h>

#include <filesystem>

using namespace vpkedit;

std::string Entry::getStem() const {
	return std::filesystem::path{this->filename}.stem().string();
}

std::string Entry::getExtension() const {
	auto ext = std::filesystem::path{this->filename}.extension().string();
	if (!ext.empty() && ext.at(0) == '.') {
		ext = ext.substr(1);
	}
	return ext;
}
