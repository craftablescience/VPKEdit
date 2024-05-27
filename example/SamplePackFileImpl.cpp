#include "SamplePackFileImpl.h"

#include <filesystem>
#include <tuple>

#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

EXAMPLE::EXAMPLE(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFile(fullFilePath_, options_) {
	// Add a new type in the PackFileType enum for this file type, and set it here
	this->type = PackFileType::UNKNOWN;
}

std::unique_ptr<PackFile> EXAMPLE::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	// Check if the file exists
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	// Create the pack file
	auto* example = new EXAMPLE{path, options};
	auto packFile = std::unique_ptr<PackFile>(example);

	// Here is where you add entries to the entries member variable
	// It's a map between a directory and a vector of entries
	// Every time an entry is added, the callback should be called if the callback exists
	std::vector<std::pair<std::string, std::string>> samplePaths{
		{"a/b/c", "skibidi_toilet.png"},
		{"d/c", "boykisser.mdl"},
		{"", "megamind.txt"},
	};
	for (auto& [dir, name] : samplePaths) {
		// The path needs to be normalized, and respect case sensitivity
		::normalizeSlashes(dir);
		if (!example->isCaseSensitive()) {
			::toLowerCase(dir);
			::toLowerCase(name);
		}

		// Create the list if it doesn't exist
		if (!example->entries.contains(dir)) {
			example->entries[dir] = {};
		}

		// Use the createNewEntry function to avoid Entry having to friend every single damn class
		Entry entry = createNewEntry();

		// The path should be the full path to the file
		entry.path = dir;
		entry.path += dir.empty() ? "" : "/";
		entry.path += name;

		// We already did it at the start, but this is how it's usually done
		//::normalizeSlashes(entry.path);
		//if (!options.allowUppercaseLettersInFilenames) {
		//	::toLowerCase(entry.path);
		//}

		// The length should be the full uncompressed length of the file data in bytes
		entry.length = 42;

		// The compressed length will be non-zero if the file is compressed, the length is in bytes
		// This can be omitted if unused, 0 is the default
		entry.compressedLength = 0;

		// This is the CRC32 of the file - a helper function to compute it is in <vpkedit/detail/CRC.h>
		// This can also be omitted if unused, 0 is the default
		entry.crc32 = 0;

		// Add the entry to the entries map
		example->entries[dir].push_back(std::move(entry));

		// Call the callback
		if (callback) {
			callback(dir, entry);
		}
	}

	return packFile;
}

std::optional<std::vector<std::byte>> EXAMPLE::readEntry(const Entry& entry) const {
	// Include this code verbatim - will likely be moved to a utility method soon
	if (entry.unbaked) {
		// Get the stored data
		for (const auto& [unbakedEntryDir, unbakedEntryList] : this->unbakedEntries) {
			for (const Entry& unbakedEntry : unbakedEntryList) {
				if (unbakedEntry.path == entry.path) {
					std::vector<std::byte> unbakedData;
					if (isEntryUnbakedUsingByteBuffer(unbakedEntry)) {
						unbakedData = std::get<std::vector<std::byte>>(getEntryUnbakedData(unbakedEntry));
					} else {
						unbakedData = ::readFileData(std::get<std::string>(getEntryUnbakedData(unbakedEntry)));
					}
					return unbakedData;
				}
			}
		}
		return std::nullopt;
	}

	// Use the contents of the entry to access the file data and return it
	// Return std::nullopt if there was an error during any step of this process - not an empty buffer!
	return std::nullopt;
}

Entry& EXAMPLE::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	// Include this verbatim
	auto filename = filename_;
	if (!this->isCaseSensitive()) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

	// Initialize the entry - set the entry properties just like in EXAMPLE::open
	entry.path = filename;
	// ...

	// Include this verbatim
	if (!this->unbakedEntries.contains(dir)) {
		this->unbakedEntries[dir] = {};
	}
	this->unbakedEntries.at(dir).push_back(entry);
	return this->unbakedEntries.at(dir).back();
}

bool EXAMPLE::bake(const std::string& outputDir_, const PackFile::Callback& callback) {
	// Get the proper file output folder (include this verbatim)
	std::string outputDir = this->getBakeOutputDir(outputDir_);
	std::string outputPath = outputDir + '/' + this->getFilename();

	// Loop over all entries and save them
	for (const auto& [entryDir, entries] : this->getBakedEntries()) {
		for (const Entry& entry : entries) {
			auto binData = this->readEntry(entry);
			if (!binData) {
				continue;
			}

			// Write data here
			// ...

			// Call the callback
			if (callback) {
				callback(entry.getParentPath(), entry);
			}
		}
	}
	// Yes this is copy-paste, you could probably turn this into a lambda and call it on both maps
	for (const auto& [entryDir, entries] : this->getUnbakedEntries()) {
		for (const Entry& entry : entries) {
			auto binData = this->readEntry(entry);
			if (!binData) {
				continue;
			}

			// Write data here
			// ...

			// Call the callback
			if (callback) {
				callback(entry.getParentPath(), entry);
			}
		}
	}

	// Call this when all the entries have been written to disk
	this->mergeUnbakedEntries();

	// Include this verbatim at the end of the function
	PackFile::setFullFilePath(outputDir);
	// Return false before this if it encounters an error
	return true;
}
