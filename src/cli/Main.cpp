#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <argparse/argparse.hpp>
#include <indicators/indeterminate_progress_bar.hpp>
#include <vpkedit/detail/Misc.h>
#include <vpkedit/detail/RSA.h>
#include <vpkedit/format/VPK.h>
#include <vpkedit/Version.h>

using namespace std::literals::string_literals;
using namespace vpkedit::detail;
using namespace vpkedit;

constexpr std::string_view ARG_OUTPUT_SHORT = "-o";
constexpr std::string_view ARG_OUTPUT_LONG = "--output";
constexpr std::string_view ARG_NO_PROGRESS_LONG = "--no-progress";
constexpr std::string_view ARG_PACK_VERSION_SHORT = "-v";
constexpr std::string_view ARG_PACK_VERSION_LONG = "--version";
constexpr std::string_view ARG_PACK_CHUNKSIZE_SHORT = "-c";
constexpr std::string_view ARG_PACK_CHUNKSIZE_LONG = "--chunksize";
constexpr std::string_view ARG_PACK_GEN_MD5_ENTRIES_LONG = "--gen-md5-entries";
constexpr std::string_view ARG_PACK_PRELOAD_SHORT = "-p";
constexpr std::string_view ARG_PACK_PRELOAD_LONG = "--preload";
constexpr std::string_view ARG_PACK_SINGLE_FILE_SHORT = "-s";
constexpr std::string_view ARG_PACK_SINGLE_FILE_LONG = "--single-file";
constexpr std::string_view ARG_PACK_SIGN_SHORT = "-k";
constexpr std::string_view ARG_PACK_SIGN_LONG = "--sign";
constexpr std::string_view ARG_GEN_GEN_KEYPAIR_LONG = "--gen-keypair";

namespace {

// Very rudimentary, doesn't handle escapes, but works fine for reading key files
std::string_view readValueForKeyInKV(std::string_view key, std::string_view kv) {
	auto index = kv.find(key);
	if (index == std::string_view::npos) {
		return "";
	}
	while (kv.size() > index && kv[index] != '\"') {
		index++;
	}
	if (index >= kv.size()) {
		return "";
	}
	while (kv.size() > index && kv[index] != '\"') {
		index++;
	}
	if (++index >= kv.size()) {
		return "";
	}
	auto beginIndex = index;
	while (kv.size() > index && kv[index] != '\"') {
		index++;
	}
	if (index >= kv.size()) {
		return "";
	}
	return std::string_view{kv.data() + beginIndex, index - beginIndex};
}

/// Pack contents of a directory into a VPK
void pack(const argparse::ArgumentParser& cli, const std::string& inputPath) {
	auto outputPath = inputPath + (cli.get<bool>("-s") || inputPath.ends_with("_dir") ? ".vpk" : "_dir.vpk");
	if (cli.is_used(ARG_OUTPUT_SHORT)) {
		if (!cli.get(ARG_OUTPUT_SHORT).ends_with(".vpk")) {
			throw std::runtime_error("Output path must be a VPK file!");
		}
		outputPath = cli.get("-o");
		if (!cli.get<bool>("-s") && !outputPath.ends_with("_dir.vpk")) {
			std::cerr << "Warning: multichunk VPK is being written without a \"_dir\" suffix (e.g. \"hl2_textures_dir.vpk\").\n"
			             "This VPK may not be able to be loaded by the Source engine or other VPK browsers!\n" << std::endl;
		}
	}

	auto noProgressBar = cli.get<bool>(ARG_NO_PROGRESS_LONG);
	auto version = static_cast<std::uint32_t>(std::stoi(cli.get(ARG_PACK_VERSION_SHORT)));
	auto preferredChunkSize = static_cast<std::uint32_t>(std::stoi(cli.get(ARG_PACK_CHUNKSIZE_SHORT)) * 1024 * 1024);
	auto generateMD5Entries = cli.get<bool>(ARG_PACK_GEN_MD5_ENTRIES_LONG);
	auto preloadExtensions = cli.get<std::vector<std::string>>(ARG_PACK_PRELOAD_SHORT);
	auto saveToDir = cli.get<bool>(ARG_PACK_SINGLE_FILE_SHORT);
	auto signPath = cli.is_used(ARG_PACK_SIGN_SHORT) ? cli.get(ARG_PACK_SIGN_SHORT) : "";

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

	auto vpk = VPK::createFromDirectoryProcedural(outputPath, inputPath, [saveToDir, &preloadExtensions, noProgressBar, &bar](const std::string& fullEntryPath) {
		int preloadBytes = 0;
		for (const auto& preloadExtension : preloadExtensions) {
			if ((std::count(preloadExtension.begin(), preloadExtension.end(), '.') > 0 && std::filesystem::path(fullEntryPath).extension().string().ends_with(preloadExtension)) ||
				std::filesystem::path(fullEntryPath).filename().string() == preloadExtension) {
				preloadBytes = VPK_MAX_PRELOAD_BYTES;
				break;
			}
		}
		if (!noProgressBar) {
			bar->tick();
		}
		return std::make_tuple(saveToDir, preloadBytes);
	}, {
		.vpk_version = version,
		.vpk_preferredChunkSize = preferredChunkSize,
		.vpk_generateMD5Entries = generateMD5Entries,
	});

	if (!noProgressBar) {
		bar->mark_as_completed();
	}

	if (!signPath.empty()) {
		if (!std::filesystem::exists(signPath) || std::filesystem::is_directory(signPath)) {
			throw std::runtime_error("Sign path must be a text file!");
		}
		auto fileData = ::readFileText(signPath);
		auto privateKeyHex = ::readValueForKeyInKV("rsa_private_key", fileData);
		if (privateKeyHex.empty()) {
			throw std::runtime_error("Could not find \"rsa_private_key\" key in file: make sure you are using the *private* key file!");
		}
		auto publicKeyHex = ::readValueForKeyInKV("rsa_public_key", fileData);
		if (publicKeyHex.empty()) {
			throw std::runtime_error("Could not find \"rsa_public_key\" key in file!");
		}
		auto privateKey = ::decodeHexString(privateKeyHex);
		auto publicKey = ::decodeHexString(publicKeyHex);

		if (!dynamic_cast<VPK*>(vpk.get())->sign(privateKey, publicKey)) {
			std::cerr << "Failed to sign VPK using private key at \"" << signPath << "\"!" << std::endl;
		} else {
			std::cout << "Signed VPK using private key at \"" << signPath << "\"." << std::endl;
		}
	}

	std::cout << "Successfully created VPK at \"" << vpk->getFilepath() << "\"." << std::endl;
}

void generateKeyPair(const std::string& inputPath) {
	if (!VPK::generateKeyPairFiles(inputPath)) {
		std::cerr << "Failed to generate public/private key files at \"" << inputPath << ".[private/public]key.vdf\"!" << std::endl;
		return;
	}
	std::cout << "Generated private/public key files at \"" << inputPath << ".[private/public]key.vdf\"." << std::endl;
	std::cout << "Remember to NEVER share a private key! The public key is fine to share." << std::endl;
}

} // namespace

int main(int argc, const char* const* argv) {
	argparse::ArgumentParser cli{std::string{PROJECT_NAME} + "cli", PROJECT_VERSION_PRETTY.data(), argparse::default_arguments::help};

#ifdef _WIN32
	// Add the Windows-specific ones because why not
	cli.set_prefix_chars("-/");
	cli.set_assign_chars("=:");
#endif

	cli.add_description("This program currently has two modes:\n"
	                    " - Pack:     Packs the contents of a given directory into a VPK.\n"
	                    " - Generate: Generates files related to VPK creation, such as a public/private keypair.\n"
	                    "Modes are exclusive to one another and automatically determined by the <path> argument, as well as the other given\n"
	                    "arguments when it is still unclear.");

	cli.add_argument("<path>")
		.help("(Pack) The directory to pack into a VPK.\n"
			  "(Gen) The name of the file(s) to generate.")
		.required();

	cli.add_argument(ARG_OUTPUT_SHORT, ARG_OUTPUT_LONG)
		.help("The path to the output VPK or directory. If unspecified, will default next to the input.");

	cli.add_argument(ARG_NO_PROGRESS_LONG)
		.help("Hide all progress bars.")
		.flag();

	cli.add_argument(ARG_PACK_VERSION_SHORT, ARG_PACK_VERSION_LONG)
		.help("(Pack) The version of the VPK. Can be 1 or 2.")
		.default_value("2")
		.choices("1", "2")
		.nargs(1);

	cli.add_argument(ARG_PACK_CHUNKSIZE_SHORT, ARG_PACK_CHUNKSIZE_LONG)
		.help("(Pack) The size of each archive in mb.")
		.default_value("200")
		.nargs(1);

	cli.add_argument(ARG_PACK_GEN_MD5_ENTRIES_LONG)
		.help("(Pack) Generate MD5 hashes for each file (v2 only).")
		.flag();

	cli.add_argument(ARG_PACK_PRELOAD_SHORT, ARG_PACK_PRELOAD_LONG)
		.help("(Pack) If a file's extension is in this list, the first kilobyte will be\n"
		      "preloaded in the directory VPK. Full file names are also supported here\n"
		      "(i.e. this would preload any files named README.md or files ending in vmt:\n"
		      "\"-p README.md vmt\"). It preloads materials by default to match Valve behavior.")
		.default_value(std::vector<std::string>{"vmt"})
		.remaining();

	cli.add_argument(ARG_PACK_SINGLE_FILE_SHORT, ARG_PACK_SINGLE_FILE_LONG)
		.help("(Pack) Pack all files into the directory VPK (single-file build).\n"
		      "Breaks the VPK if its size will be >= 4gb!")
		.flag();

	cli.add_argument(ARG_PACK_SIGN_SHORT, ARG_PACK_SIGN_LONG)
		.help("(Pack) Sign the output VPK with the key in the given private key file (v2 only).");

	cli.add_argument(ARG_GEN_GEN_KEYPAIR_LONG)
		.help("(Generate) Generate files containing public/private keys with the specified name.\n"
		      "DO NOT SHARE THE PRIVATE KEY FILE WITH ANYONE! Move it to a safe place where it will not be shipped.")
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
				std::cout << "Input path is not a directory: no action taken." << std::endl;
			}
		} else if (cli.get<bool>(ARG_GEN_GEN_KEYPAIR_LONG)) {
			::generateKeyPair(inputPath);
		} else {
			throw std::runtime_error("Given path does not exist!");
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
