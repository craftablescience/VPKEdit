#pragma once

#include <vpkedit/PackFile.h>

/*
 * --- Example Pack File Implementation ---
 *
 * This code is a template for adding a new file format to libvpkedit. Copy these two files and follow the comments!
 *
 * Note that if you are writing a read-only parser, you will need to make the following deviations:
 * - Inherit from PackFileReadOnly instead of PackFile
 * - Don't implement the bake and addEntryInternal methods
 *
 * If these instructions are followed, you should see your format appear in the VPKEdit GUI automatically.
 */

namespace vpkedit {

// Define the accepted extension(s) as constant(s)
constexpr std::string_view EXAMPLE_EXTENSION = ".example";

// All file formats need a static open method, and need to derive four methods from PackFile
class EXAMPLE : public PackFile {
public:
	// Always return a unique_ptr to PackFile so it has a uniform return type
	// If your type needs any new options, add them to PackFileOptions - it was the cleanest way to do it without messing with variants or std::any
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	// Returns the raw data the Entry points to
	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

	// Save any changes made to the opened file(s)
	bool bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) override;

protected:
	EXAMPLE(const std::string& fullFilePath_, PackFileOptions options_);

	// Adds a new entry from either a filename or a buffer
	// Again, if your type needs any new options specific to entries, add them to EntryOptions
	Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) override;

private:
	// Finally, register the open method with the extension
	// Remember since C++ is STUPID you need to add this header to PackFile.cpp as well, or this will get optimized away
	VPKEDIT_REGISTER_PACKFILE_EXTENSION(EXAMPLE_EXTENSION, &EXAMPLE::open);
};

} // namespace vpkedit
