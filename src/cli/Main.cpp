#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <argparse/argparse.hpp>
#include <vpkedit/format/VPK.h>
#include <vpkedit/Version.h>

using namespace std::literals::string_literals;
using namespace vpkedit;

namespace {

/// Pack contents of a directory into a VPK
void pack(const argparse::ArgumentParser& cli, const std::string& inputPath) {
	auto outputPath = inputPath + (cli.get<bool>("-s") || inputPath.ends_with("_dir") ? ".vpk" : "_dir.vpk");
	if (cli.is_used("-o")) {
		if (!cli.get("-o").ends_with(".vpk")) {
			throw std::runtime_error("Output path must be a VPK file!");
		}
		outputPath = cli.get("-o");
		if (!cli.get<bool>("-s") && !outputPath.ends_with("_dir.vpk")) {
			std::cerr << "Warning: multichunk VPK is being written without a \"_dir\" suffix (e.g. \"hl2_textures_dir.vpk\").\n"
			             "This VPK may not be able to be loaded by the Source engine or other VPK browsers!\n" << std::endl;
		}
	}

	bool saveToDir = cli.get<bool>("-s");
	auto preloadExtensions = cli.get<std::vector<std::string>>("-p");
	auto version = static_cast<std::uint32_t>(std::stoi(cli.get("-v")));
	auto preferredChunkSize = static_cast<std::uint32_t>(std::stoi(cli.get("-c")) * 1024 * 1024);
	auto generateMD5Entries = cli.get<bool>("--gen-md5-entries");

	auto vpk = VPK::createFromDirectoryProcedural(outputPath, inputPath, [saveToDir, &preloadExtensions](const std::string& fullEntryPath) {
		int preloadBytes = 0;
		for (const auto& preloadExtension : preloadExtensions) {
			if ((std::count(preloadExtension.begin(), preloadExtension.end(), '.') > 0 && std::filesystem::path(fullEntryPath).extension().string().ends_with(preloadExtension)) ||
			    std::filesystem::path(fullEntryPath).filename().string() == preloadExtension) {
				preloadBytes = VPK_MAX_PRELOAD_BYTES;
				break;
			}
		}
		return std::make_tuple(saveToDir, preloadBytes);
	}, {
		.vpk_version = version,
		.vpk_preferredChunkSize = preferredChunkSize,
		.vpk_generateMD5Entries = generateMD5Entries,
	});
	std::cout << "Successfully created VPK at \"" << vpk->getFilepath() << std::endl;
}

} // namespace

int main(int argc, const char* const* argv) {
	argparse::ArgumentParser cli{std::string{PROJECT_NAME} + "cli", PROJECT_VERSION_PRETTY.data(), argparse::default_arguments::help};

	cli.add_epilog("Program details:\n"
	               "                    /$$                       /$$ /$$   /$$        \n"
	               "                   | $$                      | $$|__/  | $$        \n"
	               " /$$    /$$/$$$$$$ | $$   /$$  /$$$$$$   /$$$$$$$ /$$ /$$$$$$      \n"
	               "|  $$  /$$/$$__  $$| $$  /$$/ /$$__  $$ /$$__  $$| $$|_  $$_/      \n"
	               " \\  $$/$$/ $$  \\ $$| $$$$$$/ | $$$$$$$$| $$  | $$| $$  | $$      \n"
	               "  \\  $$$/| $$  | $$| $$_  $$ | $$_____/| $$  | $$| $$  | $$ /$$   \n"
	               "   \\  $/ | $$$$$$$/| $$ \\  $$|  $$$$$$$|  $$$$$$$| $$  |  $$$$/  \n"
	               "    \\_/  | $$____/ |__/  \\__/ \\_______/ \\_______/|__/   \\____/\n"
	               "         | $$                                                      \n"
	               "         | $$             version v"s + PROJECT_VERSION_PRETTY.data() + "\n"
	               "         |__/                                                      \n"
	               "                                                                   \n"
	               "Created by craftablescience. Contributors and libraries used are   \n"
	               "listed in CREDITS.md. " + PROJECT_NAME_PRETTY.data() + " is licensed under the MIT License.");

#ifdef _WIN32
	// Add the Windows-specific ones because why not
	cli.set_prefix_chars("-/");
	cli.set_assign_chars("=:");
#endif

	cli.add_argument("<path>")
		.help("The directory to pack into a VPK, or the VPK to extract into a directory.")
		.required();

	cli.add_argument("-o", "--output")
		.help("The path to the output VPK or directory. If unspecified, will default next to the input.");

	cli.add_argument("-v", "--version")
		.help("(Pack) The version of the VPK. Can be 1 or 2.")
		.default_value("2")
		.choices("1", "2")
		.nargs(1);

	cli.add_argument("-c", "--chunksize")
		.help("(Pack) The size of each archive in mb.")
		.default_value("200")
		.nargs(1);

	cli.add_argument("--gen-md5-entries")
		.help("(Pack) Generate MD5 hashes for each file (v2 only).")
		.flag();

	cli.add_argument("-p", "--preload")
		.help("(Pack) If a file's extension is in this list, the first kilobyte will be\n"
		      "preloaded in the directory VPK. Full file names are also supported here\n"
		      "(i.e. this would preload any files named README.md or files ending in vmt:\n"
		      "\"-p README.md vmt\"). It preloads materials by default to match Valve\n"
		      "behavior.")
		.default_value(std::vector<std::string>{"vmt"})
		.remaining();

	cli.add_argument("-s", "--single-file")
		.help("(Pack) Pack all files into the directory VPK (single-file build).\n"
		      "Breaks the VPK if its size will be >= 4gb!")
		.flag();

	try {
		cli.parse_args(argc, argv);

		std::string inputPath{cli.get("<path>")};
		if (!std::filesystem::exists(inputPath)) {
			throw std::runtime_error("Given path does not exist!");
		}
		if (inputPath.ends_with('/') || inputPath.ends_with('\\')) {
			inputPath.pop_back();
		}

		if (std::filesystem::status(inputPath).type() == std::filesystem::file_type::directory) {
			::pack(cli, inputPath);
		}
	} catch (const std::exception& e) {
		if (argc > 1) {
			std::cerr << e.what() << "\n\n";
			std::cerr << cli << std::endl;
		} else {
			std::cout << cli << std::endl;
		}
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
