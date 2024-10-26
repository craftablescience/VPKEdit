#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <argparse/argparse.hpp>
#include <indicators/indeterminate_progress_bar.hpp>
#include <vpkpp/vpkpp.h>

#include <Version.h>

using namespace std::literals::string_literals;
using namespace vpkpp;

#define ARG_S(name, short_, long_)                          \
	constexpr std::string_view ARG_##name##_SHORT = short_; \
	constexpr std::string_view ARG_##name##_LONG  = long_
#define ARG_L(name, long_) \
	constexpr std::string_view ARG_##name##_LONG  = long_

ARG_S(OUTPUT,             "-o", "--output");
ARG_S(TYPE,               "-t", "--type");
ARG_L(NO_PROGRESS,              "--no-progress");
ARG_S(VERSION,            "-v", "--version");
ARG_S(CHUNKSIZE,          "-c", "--chunksize");
ARG_S(COMPRESSION_METHOD, "-m", "--compression-method");
ARG_S(COMPRESSION_LEVEL,  "-x", "--compression-level");
ARG_L(GEN_MD5_ENTRIES,          "--gen-md5-entries");
ARG_L(ADD_FILE,                 "--add-file");
ARG_L(REMOVE_FILE,              "--remove-file");
ARG_S(PRELOAD,            "-p", "--preload");
ARG_S(SINGLE_FILE,        "-s", "--single-file");
ARG_S(EXTRACT,            "-e", "--extract");
ARG_L(GEN_KEYPAIR,              "--gen-keypair");
ARG_L(FILE_TREE,                "--file-tree");
ARG_S(SIGN,               "-k", "--sign");
ARG_L(VERIFY_CHECKSUMS,         "--verify-checksums");
ARG_L(VERIFY_SIGNATURE,         "--verify-signature");

#undef ARG_S
#undef ARG_L
#define ARG_S(name) ARG_##name##_SHORT
#define ARG_L(name) ARG_##name##_LONG
#define ARG_P(name) ARG_S(name), ARG_L(name)

namespace {

/// Convert compression method to type enum
[[nodiscard]] EntryCompressionType compressionMethodStringToCompressionType(std::string_view in) {
	using enum EntryCompressionType;
	if (in == "deflate") {
		return DEFLATE;
	} else if (in == "bzip2") {
		return BZIP2;
	} else if (in == "lzma") {
		return LZMA;
	} else if (in == "zstd") {
		return ZSTD;
	} else if (in == "xz") {
		return XZ;
	}
	// "none"
	return NO_COMPRESS;
}

/// Extract file(s) from an existing pack file
void extract(const argparse::ArgumentParser& cli, const std::string& inputPath) {
	auto packFile = PackFile::open(inputPath);
	if (!packFile) {
		std::cerr << "Could not open the pack file at \"" << inputPath << "\": it failed to load!" << std::endl;
	}

	auto extractPath = cli.get(ARG_S(EXTRACT));
	if (extractPath == "/") {
		// Extract everything
		auto outputPath = std::filesystem::path{inputPath}.parent_path().string();
		if (cli.is_used(ARG_S(OUTPUT))) {
			outputPath = cli.get(ARG_S(OUTPUT));
		}
		if (!std::filesystem::exists(outputPath) || !std::filesystem::is_directory(outputPath)) {
			std::cerr << "Output location must be an existing directory!" << std::endl;
			return;
		}
		if (!packFile->extractAll(outputPath)) {
			std::cerr
					<< "Could not extract pack file contents to \"" << outputPath << "\"!\n"
					<< "Please ensure that a game or another application is not using the file, and that you have sufficient permissions to write to the output location."
					<< std::endl;
		}
		std::cout << "Extracted pack file contents under \"" << outputPath << "\"." << std::endl;
	} else if (extractPath.ends_with('/')) {
		// Extract directory
		auto outputPath = std::filesystem::path{inputPath}.parent_path().string();
		if (cli.is_used(ARG_S(OUTPUT))) {
			outputPath = cli.get(ARG_S(OUTPUT));
		}
		if (!std::filesystem::exists(outputPath) || !std::filesystem::is_directory(outputPath)) {
			std::cerr << "Output location must be an existing directory!" << std::endl;
			return;
		}
		if (!packFile->extractDirectory(extractPath, outputPath)) {
			std::cerr
					<< "Some or all files were unable to be extracted to \"" << outputPath << "\"!\n"
					<< "Please ensure that a game or another application is not using the file, and that you have sufficient permissions to write to the output location."
					<< std::endl;
		}
		std::cout << "Extracted directory under \"" << outputPath << "\"." << std::endl;
	} else {
		// Extract file
		auto outputPath = (std::filesystem::path{inputPath}.parent_path() / std::filesystem::path{extractPath}.filename()).string();
		if (cli.is_used(ARG_S(OUTPUT))) {
			outputPath = cli.get(ARG_S(OUTPUT));
		}
		auto entry = packFile->findEntry(extractPath);
		if (!entry) {
			std::cerr << "Could not find file at \"" << extractPath << "\" in the pack file!" << std::endl;
			return;
		}
		if (!packFile->extractEntry(extractPath, outputPath)) {
			std::cerr
				<< "Could not extract file at \"" << extractPath << "\" to \"" << outputPath << "\"!\n"
				<< "Please ensure that a game or another application is not using the file, and that you have sufficient permissions to write to the output location."
				<< std::endl;
		}
		std::cout << "Extracted file at \"" << extractPath << "\" to \"" << outputPath << "\"." << std::endl;
	}
}

/// Print the file tree of an existing pack file
void fileTree(const std::string& inputPath) {
	auto packFile = PackFile::open(inputPath);
	if (!packFile) {
		std::cerr << "Could not open the pack file at \"" << inputPath << "\": it failed to load!" << std::endl;
	}

	// todo: make this more tree-like
	packFile->runForAllEntries([](const std::string& path, const Entry& entry) {
		std::cout << path << std::endl;
	});
}

/// Generate private/public key files
void generateKeyPair(const std::string& inputPath) {
	if (!VPK::generateKeyPairFiles(inputPath)) {
		std::cerr << "Failed to generate public/private key files at \"" << inputPath << ".[private/public]key.vdf\"!" << std::endl;
		return;
	}
	std::cout << "Generated private/public key files at \"" << inputPath << ".[private/public]key.vdf\"." << std::endl;
	std::cout << "Remember to NEVER share a private key! The public key is fine to share." << std::endl;
}

/// Edit the contents of an existing pack file
void edit(const argparse::ArgumentParser& cli, const std::string& inputPath) {
	auto packFile = PackFile::open(inputPath);
	if (!packFile) {
		std::cerr << "Could not open the pack file at \"" << inputPath << "\": it failed to load!" << std::endl;
		return;
	}

	auto compressionMethod = ::compressionMethodStringToCompressionType(cli.get<std::string>(ARG_S(COMPRESSION_METHOD)));
	auto compressionLevel = static_cast<int8_t>(std::stoi(cli.get<std::string>(ARG_S(COMPRESSION_LEVEL))));
	auto generateMD5Entries = cli.get<bool>(ARG_L(GEN_MD5_ENTRIES));

	if (cli.is_used(ARG_L(ADD_FILE))) {
		auto args = cli.get<std::vector<std::string>>(ARG_L(ADD_FILE));
		if (!std::filesystem::exists(args[0])) {
			std::cerr << "File at \"" << args[0] << "\" does not exist! Cannot add to pack file." << std::endl;
		} else {
			packFile->addEntry(args[1], args[0], {});
			packFile->bake("", {
				.zip_compressionTypeOverride = compressionMethod,
				.zip_compressionStrength = compressionLevel,
				.vpk_generateMD5Entries = generateMD5Entries,
			}, nullptr);
			std::cout << "Added file at \"" << args[0] << "\" to the pack file at path \"" << args[1] << "\"." << std::endl;
		}
	}

	if (cli.is_used(ARG_L(REMOVE_FILE))) {
		auto path = cli.get(ARG_L(REMOVE_FILE));
		if (!packFile->removeEntry(path)) {
			std::cerr << "Unable to remove file at \"" << path << "\" from the pack file!\n" <<
					"Check the file exists in the pack file and the path is spelled correctly." << std::endl;
		} else {
			packFile->bake("", {
				.zip_compressionStrength = compressionLevel,
				.vpk_generateMD5Entries = generateMD5Entries,
			}, nullptr);
			std::cout << "Removed file at \"" << path << "\" from the pack file." << std::endl;
		}
	}
}

/// Sign an existing VPK
void sign(const argparse::ArgumentParser& cli, const std::string& inputPath) {
	auto saveToDir = cli.get<bool>(ARG_S(SINGLE_FILE));
	auto signPath = cli.is_used(ARG_S(SIGN)) ? cli.get(ARG_S(SIGN)) : "";

	if (saveToDir) {
		std::cerr << "Warning: Signed VPKs that contain files will not be treated as signed by the Source engine!" << std::endl;
		std::cerr << "Remove the " << ARG_S(SINGLE_FILE) << " / " << ARG_L(SINGLE_FILE) << " parameter for best results." << std::endl;
	}

	auto vpk = VPK::open(inputPath);
	if (!vpk || !dynamic_cast<VPK*>(vpk.get())->sign(signPath)) {
		std::cerr << "Failed to sign VPK using private key file at \"" << signPath << "\"!" << std::endl;
		std::cerr << "Check that the file exists and it contains both the private key and public key." << std::endl;
	} else {
		std::cout << "Signed VPK using private key at \"" << signPath << "\"." << std::endl;
	}
}

/// Verify checksums and/or signature are valid for an existing pack file
void verify(const argparse::ArgumentParser& cli, const std::string& inputPath) {
	auto packFile = PackFile::open(inputPath);
	if (!packFile) {
		std::cerr << "Could not open then pack file at \"" << inputPath << "\": it failed to load!" << std::endl;
		return;
	}

	if (cli.is_used(ARG_L(VERIFY_CHECKSUMS))) {
		if (cli.get(ARG_L(VERIFY_CHECKSUMS)) == "all" || cli.get(ARG_L(VERIFY_CHECKSUMS)) == "overall") {
			if (!packFile->hasPackFileChecksum()) {
				std::cout << "This pack file has no overall checksum(s)." << std::endl;
			} else if (packFile->verifyPackFileChecksum()) {
				std::cout << "Overall pack file checksums match their expected values." << std::endl;
			} else {
				std::cerr << "One or more of the pack file overall checksums do not match the expected value(s)!" << std::endl;
			}
		}
		if (cli.get(ARG_L(VERIFY_CHECKSUMS)) == "all" || cli.get(ARG_L(VERIFY_CHECKSUMS)) == "files") {
			if (!packFile->hasEntryChecksums()) {
				std::cout << "This pack file format does not store checksums per file." << std::endl;
			} else if (auto entries = packFile->verifyEntryChecksums(); entries.empty()) {
				std::cout << "All file checksums match their expected values." << std::endl;
			} else {
				std::cerr << "Some file checksums do not match their expected values!" << std::endl;
				std::cerr << "Files that failed to validate:" << std::endl;
				for (const auto& entryPath : entries) {
					std::cerr << entryPath << std::endl;
				}
			}
		}
	}

	if (cli.is_used(ARG_L(VERIFY_SIGNATURE))) {
		if (!packFile->hasPackFileSignature()) {
			std::cout << "Pack file does not have a signature." << std::endl;
		} else if (packFile->verifyPackFileSignature()) {
			std::cout << "Pack file signature is valid." << std::endl;
		} else {
			std::cerr << "Pack file signature is invalid!" << std::endl;
		}
	}
}

/// Pack contents of a directory into a new pack file
void pack(const argparse::ArgumentParser& cli, const std::string& inputPath) {
	auto type = cli.get<std::string>(ARG_S(TYPE));
	std::string extension = '.' + type;
	if (type == "vpk_vtmb") {
		extension = ".vpk";
	}

	std::string outputPath = inputPath;
	if (type == "fpx" || type == "vpk") {
		outputPath += (cli.get<bool>(ARG_S(SINGLE_FILE)) || inputPath.ends_with("_dir") ? "" : "_dir") + extension;
	} else {
		outputPath += extension;
	}
	if (cli.is_used(ARG_S(OUTPUT))) {
		outputPath = cli.get(ARG_S(OUTPUT));
		if (!outputPath.ends_with(extension)) {
			const auto fsPath = std::filesystem::path{outputPath};
			outputPath = (fsPath.parent_path() / fsPath.stem()).string() + extension;
		}
		if ((type == "fpx" || type == "vpk") && !cli.get<bool>(ARG_S(SINGLE_FILE)) && !outputPath.ends_with("_dir" + extension)) {
			std::cerr << "Warning: multichunk FPX/VPK is being written without a \"_dir\" suffix (e.g. \"hl2_textures_dir.vpk\").\n"
			             "The Source engine may not be able to load this file!" << std::endl;
		}
	}

	auto noProgressBar = cli.get<bool>(ARG_L(NO_PROGRESS));
	auto version = static_cast<std::uint32_t>(std::stoi(cli.get(ARG_S(VERSION))));
	auto preferredChunkSize = static_cast<std::uint32_t>(std::stoi(cli.get(ARG_S(CHUNKSIZE))) * 1024 * 1024);
	auto compressionMethod = ::compressionMethodStringToCompressionType(cli.get<std::string>(ARG_S(COMPRESSION_METHOD)));
	auto compressionLevel = static_cast<int8_t>(std::stoi(cli.get<std::string>(ARG_S(COMPRESSION_LEVEL))));
	auto generateMD5Entries = cli.get<bool>(ARG_L(GEN_MD5_ENTRIES));
	auto preloadExtensions = cli.get<std::vector<std::string>>(ARG_S(PRELOAD));
	auto saveToDir = cli.get<bool>(ARG_S(SINGLE_FILE));
	auto fileTree = cli.get<bool>(ARG_L(FILE_TREE));
	auto signPath = cli.is_used(ARG_S(SIGN)) ? cli.get(ARG_S(SIGN)) : "";
	auto shouldVerify = cli.is_used(ARG_L(VERIFY_CHECKSUMS)) || cli.is_used(ARG_L(VERIFY_SIGNATURE));

	std::unique_ptr<indicators::IndeterminateProgressBar> bar;
	if (!noProgressBar) {
		bar = std::make_unique<indicators::IndeterminateProgressBar>(
			indicators::option::BarWidth{40},
			indicators::option::Start{"["},
			indicators::option::Fill{"·"},
			indicators::option::Lead{"<==>"},
			indicators::option::End{"]"},
			indicators::option::PostfixText{"Packing files..."}
		);
	}

	std::unique_ptr<PackFile> packFile;
	if (type == "bmz" || type == "zip") {
		packFile = ZIP::create(outputPath);
	} else if (type == "fpx") {
		packFile = FPX::create(outputPath);
		if (auto* fpx = dynamic_cast<FPX*>(packFile.get())) {
			fpx->setChunkSize(preferredChunkSize);
		}
	} else if (type == "pak") {
		packFile = PAK::create(outputPath);
	} else if (type == "pck") {
		packFile = PCK::create(outputPath, version);
	} else if (type == "vpk_vtmb") {
		packFile = VPK_VTMB::create(outputPath);
	} else if (type == "vpk") {
		packFile = VPK::create(outputPath, version);
		if (auto* vpk = dynamic_cast<VPK*>(packFile.get())) {
			vpk->setChunkSize(preferredChunkSize);
		}
	} else if (type == "wad3") {
		packFile = WAD3::create(outputPath);
	}
	if (!packFile) {
		std::cerr << "Failed to create pack file!" << std::endl;
		if (!noProgressBar) {
			bar->mark_as_completed();
		}
		return;
	}

	packFile->addDirectory("", inputPath, [saveToDir, &preloadExtensions](const std::string& path) -> EntryOptions {
		uint32_t preloadBytes = 0;
		for (const auto& preloadExtension : preloadExtensions) {
			if ((std::count(preloadExtension.begin(), preloadExtension.end(), '.') > 0 && std::filesystem::path{path}.extension().string().ends_with(preloadExtension)) || std::filesystem::path{path}.filename().string() == preloadExtension) {
				preloadBytes = VPK_MAX_PRELOAD_BYTES;
				break;
			}
		}
		return {
			.vpk_saveToDirectory = saveToDir,
			.vpk_preloadBytes = preloadBytes,
		};
	});
	packFile->bake("", {
		.zip_compressionTypeOverride = compressionMethod,
		.zip_compressionStrength = compressionLevel,
		.vpk_generateMD5Entries = generateMD5Entries,
	}, [noProgressBar, &bar](const std::string& path, const Entry& entry) {
		if (!noProgressBar) {
			bar->tick();
		}
	});

	if (!noProgressBar) {
		bar->mark_as_completed();
	}

	if (fileTree) {
		::fileTree(outputPath);
	}
	if (!signPath.empty()) {
		::sign(cli, outputPath);
	}
	if (shouldVerify) {
		::verify(cli, outputPath);
	}

	std::cout << "Successfully created pack file at \"" << packFile->getFilepath() << "\"." << std::endl;
}

} // namespace

int main(int argc, const char* const* argv) {
	argparse::ArgumentParser cli{std::string{PROJECT_NAME} + "cli", PROJECT_VERSION_PRETTY.data(), argparse::default_arguments::help};

#ifdef _WIN32
	// Add the Windows-specific ones because why not
	cli.set_prefix_chars("-/");
	cli.set_assign_chars("=:");
#endif

	cli.add_description("This program currently has seven modes:\n"
	                    " - Pack:     Packs the contents of a given directory into a new pack file.\n"
	                    " - Extract:  Extracts files from the given pack file.\n"
	                    " - Generate: Generates files related to VPK creation, such as a public/private keypair.\n"
	                    " - Modify:   Edits the contents of the given pack file.\n"
	                    " - Preview:  Prints the file tree of the given pack file to the console. Can also be combined\n"
	                    "             with Pack mode to print the file tree of the new pack file.\n"
	                    " - Sign:     Signs an existing VPK. Can also be combined with Pack mode to sign the new VPK.\n"
	                    " - Verify:   Verify the given pack file's checksums and/or signature. If used together with\n"
	                    "             Pack or Sign modes, it will verify the pack file after the other modes are finished.\n"
	                    "Modes are automatically determined by the <path> argument, as well as the other given arguments\n"
	                    "when it is still unclear. Almost all modes are compatible with each other, and will run in the\n"
	                    "most logical sequence possible.");

	cli.add_argument("<path>")
		.help("(Pack)     The directory to pack the contents of into a new pack file.\n"
		      "(Extract)  The path to the pack file to extract the contents of.\n"
		      "(Generate) The name of the file(s) to generate.\n"
		      "(Modify)   The path to the pack file to edit the contents of.\n"
		      "(Preview)  The path to the pack file to print the file tree of.\n"
		      "(Sign)     The path to the VPK to sign.\n"
		      "(Verify)   The path to the pack file to verify the contents of.")
		.required();

	cli.add_argument(ARG_P(OUTPUT))
		.help("The path to the output pack file, directory, or file. If unspecified, will default next to the input path.");

	cli.add_argument(ARG_P(TYPE))
		.help("(Pack) The type of the output pack file.")
		.default_value("vpk")
		.choices("bmz", "fpx", "pak", "pck", "vpk_vtmb", "vpk", "wad3", "zip")
		.nargs(1);

	cli.add_argument(ARG_L(NO_PROGRESS))
		.help("Hide all progress bars.")
		.flag();

	cli.add_argument(ARG_P(VERSION))
		.help("(Pack) The version of the PCK/VPK.")
		.default_value("2")
		.choices("0", "1", "2", "54")
		.nargs(1);

	cli.add_argument(ARG_P(CHUNKSIZE))
		.help("(Pack) The size of each archive in mb.")
		.default_value("200")
		.nargs(1);

	cli.add_argument(ARG_P(COMPRESSION_METHOD))
		.help("(Pack) The method of compression to apply. BSP only supports \"none\" and \"lzma\".\n"
		      "ZIP supports all options. Option is unused for all other pack file types.")
		.choices("none", "deflate", "bzip2", "lzma", "zstd", "xz")
		.default_value("none")
		.nargs(1);

	cli.add_argument(ARG_P(COMPRESSION_LEVEL))
		.help("(Pack) The level of compression to apply. (BSP, VPK v54, and ZIP only).")
		.default_value("5")
		.nargs(1);

	cli.add_argument(ARG_L(GEN_MD5_ENTRIES))
		.help("(Pack) Generate MD5 hashes for each file (VPK v2, v54 only).")
		.flag();

	cli.add_argument(ARG_L(ADD_FILE))
		.help("(Modify) Add the specified file to the pack file with the given path.")
		.nargs(2);

	cli.add_argument(ARG_L(REMOVE_FILE))
		.help("(Modify) Remove the specified file at the given path from the pack file.")
		.nargs(1);

	cli.add_argument(ARG_P(PRELOAD))
		.help("(Pack) If a file's extension is in this list, the first kilobyte will be\n"
		      "preloaded in the directory FPX/VPK. Full file names are also supported here\n"
		      "(i.e. this would preload any files named README.md or files ending in vmt:\n"
		      "\"-p README.md vmt\"). It preloads materials by default to match Valve behavior.")
		.default_value(std::vector<std::string>{"vmt"})
		.remaining();

	cli.add_argument(ARG_P(SINGLE_FILE))
		.help("(Pack) Pack all files into the directory FPX/VPK (single-file build).\n"
		      "Breaks the pack file if its size will be >= 4gb!")
		.flag();

	cli.add_argument(ARG_P(EXTRACT))
		.help("(Extract) Extracts the given file or directory, or the entire pack file.\n"
		      "If given a file path, it will try to read that file and save it to the output path.\n"
		      "If given a path ending in a forward slash, it will try to extract that directory\n"
		      "to the given output directory.\n"
		      "If given a single slash, it will extract the entire pack file under the given output\n"
		      "directory.")
		.default_value("/")
		.nargs(1);

	cli.add_argument(ARG_L(GEN_KEYPAIR))
		.help("(Generate) Generate files containing public/private keys with the specified name.\n"
		      "DO NOT SHARE THE PRIVATE KEY FILE WITH ANYONE! Move it to a safe place where it\n"
		      "will not be shipped.")
		.flag();

	cli.add_argument(ARG_L(FILE_TREE))
		.help("(Preview) Prints the file tree of the given pack file to the console.")
		.flag();

	cli.add_argument(ARG_P(SIGN))
	    .help("(Pack) Sign the output VPK with the key in the given private key file (v2 only).\n"
	          "(Sign) Sign the VPK with the key in the given private key file (v2 only).");

	cli.add_argument(ARG_L(VERIFY_CHECKSUMS))
		.help(R"((Verify) Verify the pack file's checksums. Can be "files", "overall", or "all" (without quotes).)")
		.choices("files", "overall", "all")
		.nargs(1);

	cli.add_argument(ARG_L(VERIFY_SIGNATURE))
		.help("(Verify) Verify the pack file's signature if it exists.")
		.flag();

	cli.add_epilog(R"(Program details:                                               )"        "\n"
	               R"(                    /$$                       /$$ /$$   /$$    )"        "\n"
	               R"(                   | $$                      | $$|__/  | $$    )"        "\n"
	               R"( /$$    /$$/$$$$$$ | $$   /$$  /$$$$$$   /$$$$$$$ /$$ /$$$$$$  )"        "\n"
	               R"(|  $$  /$$/$$__  $$| $$  /$$/ /$$__  $$ /$$__  $$| $$|_  $$_/  )"        "\n"
	               R"( \  $$/$$/ $$  \ $$| $$$$$$/ | $$$$$$$$| $$  | $$| $$  | $$    )"        "\n"
	               R"(  \  $$$/| $$  | $$| $$_  $$ | $$_____/| $$  | $$| $$  | $$ /$$)"        "\n"
	               R"(   \  $/ | $$$$$$$/| $$ \  $$|  $$$$$$$|  $$$$$$$| $$  |  $$$$/)"        "\n"
	               R"(    \_/  | $$____/ |__/  \__/ \_______/ \_______/|__/   \____/ )"        "\n"
	               R"(         | $$                                                  )"        "\n"
	               R"(         | $$             version v)"s + PROJECT_VERSION_PRETTY.data() + "\n"
	               R"(         |__/                                                  )"        "\n"
	               R"(                                                               )"        "\n"
	               "Created by craftablescience. Contributors and libraries used are"          "\n"
	               "listed in CREDITS.md. " + PROJECT_NAME_PRETTY.data() + " is licensed under the MIT License.");

	try {
		cli.parse_args(argc, argv);

		std::string inputPath{cli.get("<path>")};
		if (inputPath.ends_with('/') || inputPath.ends_with('\\')) {
			inputPath.pop_back();
		}

		if (std::filesystem::exists(inputPath)) {
			if (std::filesystem::status(inputPath).type() == std::filesystem::file_type::directory) {
				::pack(cli, inputPath);
			} else {
				bool foundAction = false;
				if (cli.is_used(ARG_S(EXTRACT))) {
					foundAction = true;
					::extract(cli, inputPath);
				}
				if (cli.is_used(ARG_L(FILE_TREE))) {
					foundAction = true;
					::fileTree(inputPath);
				}
				if (cli.is_used(ARG_L(ADD_FILE)) || cli.is_used(ARG_L(REMOVE_FILE))) {
					foundAction = true;
					::edit(cli, inputPath);
				}
				if (cli.is_used(ARG_S(SIGN))) {
					foundAction = true;
					::sign(cli, inputPath);
				}
				if (cli.is_used(ARG_L(VERIFY_CHECKSUMS)) || cli.is_used(ARG_L(VERIFY_SIGNATURE))) {
					foundAction = true;
					::verify(cli, inputPath);
				}
				if (!foundAction) {
					throw std::runtime_error{"No action taken! Add some arguments to clarify your intent."};
				}
			}
		} else if (cli.get<bool>(ARG_L(GEN_KEYPAIR))) {
			::generateKeyPair(inputPath);
		} else {
			throw std::runtime_error{"Given path does not exist!"};
		}
	} catch (const std::exception& e) {
		if (argc > 1) {
			std::cerr << e.what() << '\n' << std::endl;
			std::cerr << cli << std::endl;
		} else {
			std::cout << cli << std::endl;
		}
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
