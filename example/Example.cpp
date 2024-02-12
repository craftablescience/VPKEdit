#include <iostream>

#include <vpkedit/PackFile.h>

#include "SamplePackFileImpl.h"

int main() {
	std::cout << "Supported file types:" << std::endl;
	for (const auto& ext : vpkedit::PackFile::getSupportedFileTypes()) {
		std::cout << ext << std::endl;
	}

	// This won't work lol
	(void) vpkedit::PackFile::open("file.example", {});
}
