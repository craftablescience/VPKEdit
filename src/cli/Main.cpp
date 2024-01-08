#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <argparse/argparse.hpp>
#include <vpkedit/Version.h>
#include <vpkedit/VPK.h>

using namespace std::literals::string_literals;
using namespace vpkedit;

int main(int argc, const char* const* argv) {
	argparse::ArgumentParser cli{std::string{PROJECT_NAME} + "cli", PROJECT_VERSION.data(), argparse::default_arguments::help};

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
	               "         | $$             version v"s + PROJECT_VERSION.data() +  "\n"
				   "         |__/                                                      \n"
	               "                                                                   \n"
	               "Created by craftablescience. Contributors and libraries used are   \n"
	               "listed in CREDITS.md. VPKEdit is licensed under the MIT License.");

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

	cli.add_argument("-m", "--multichunk")
		.help("(Pack) The size of each archive in mb.")
		.default_value("200")
		.nargs(1);

	cli.add_argument("-s", "--single-file")
		.help("(Pack) Pack all files into the directory VPK (single-file build).\n"
			  "May break if the total file size is larger than ~2gb.")
		.flag();

	// todo: list files
	//cli.add_argument("-l", "--list")
	//   .help("(Extract) Rather than extracting the files, print a list of files the VPK contains.")
	//   .flag();

	try {
		cli.parse_args(argc, argv);

		std::string inputPath{cli.get("<path>")};
		if (!std::filesystem::exists(inputPath)) {
			throw std::runtime_error("Given path does not exist!");
		}
		if (std::filesystem::status(inputPath).type() == std::filesystem::file_type::directory) {
			// Pack
			auto outputPath = inputPath + (cli.get<bool>("-s") ? ".vpk" : "_dir.vpk");
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
			auto vpk = VPK::createFromDirectory(outputPath, inputPath, cli.get<bool>("-s"), {
				.version = static_cast<std::uint32_t>(std::stoi(cli.get("-v"))),
				.preferredChunkSize = static_cast<std::uint32_t>(std::stoi(cli.get("-m")) * 1024 * 1024),
			});
			std::cout << "Successfully created VPK at \"" << vpk.getRealFilename() << ".vpk\"" << std::endl;
			return EXIT_SUCCESS;
		} /*else if (path.ends_with(".vpk")) {
			// Extract
			// todo: extract files
			return EXIT_SUCCESS;
		}*/
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
