// ReSharper disable CppDFATimeOver
// ReSharper disable CppDeclaratorNeverUsed
// ReSharper disable CppUnusedIncludeDirective
// ReSharper disable CppUseStructuredBinding

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

#if defined(_WIN32)
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

#include <argparse/argparse.hpp>
#include <bsppp/PakLump.h>
#include <sourcepp/String.h>
#include <vpkpp/vpkpp.h>

#include "Config.hpp"

using namespace std::literals;

namespace {

[[nodiscard]] bool runningInTTY() {
#if defined(_WIN32)
	static const bool check = _isatty(_fileno(stdout)) && _isatty(_fileno(stderr));
#elif defined(__unix__) || defined(__APPLE__)
	static const bool check = isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
#else
	static constexpr bool check = true;
#endif
	return check;
}

template<typename duration = std::chrono::milliseconds>
class ElapsedTime {
	using clock = std::chrono::steady_clock;

public:
	ElapsedTime() : start{std::chrono::time_point_cast<duration>(clock::now())} {}

	[[nodiscard]] duration get() const noexcept {
		return std::chrono::duration_cast<duration>(std::chrono::time_point_cast<duration>(clock::now()) - this->start);
	}

protected:
	std::chrono::time_point<clock, duration> start;
};

struct tfout_t {
	static inline bool QUIET = false;
	tfout_t& operator<<(const auto& x) {
		if (!QUIET) std::cout << x;
		return *this;
	}
} tfout;

struct tfout_override_t {
	tfout_override_t& operator<<(const auto& x) {
		std::cout << x;
		return *this;
	}
} tfout_override;

struct tferr_t {
	static inline bool QUIET = false;
	tferr_t& operator<<(const auto& x) {
		if (!QUIET) std::cerr << x;
		return *this;
	}
} tferr;

struct tfendl_t {} tfendl;

// ReSharper disable once CppDeclaratorNeverUsed
template<> tfout_t& tfout_t::operator<<<tfendl_t>(const tfendl_t&) {
	if (!QUIET) std::cout << std::endl;
	return *this;
}

// ReSharper disable once CppDeclaratorNeverUsed
template<> tfout_override_t& tfout_override_t::operator<<<tfendl_t>(const tfendl_t&) {
	std::cout << std::endl;
	return *this;
}

// ReSharper disable once CppDeclaratorNeverUsed
template<> tferr_t& tferr_t::operator<<<tfendl_t>(const tfendl_t&) {
	if (!QUIET) std::cerr << std::endl;
	return *this;
}

[[nodiscard]] std::string encodeBase64(std::span<const std::byte> input) {
	static constexpr auto encodeBlock = [](std::byte a, std::byte b, std::byte c) -> std::array<char, 4> {
		static constexpr std::string_view B64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		const auto bits = static_cast<uint32_t>(a) << 16 | static_cast<uint32_t>(b) << 8 | static_cast<uint32_t>(c) << 0;
		return {
			B64_CHARS[bits >> 18 & 0b111111],
			B64_CHARS[bits >> 12 & 0b111111],
			B64_CHARS[bits >>  6 & 0b111111],
			B64_CHARS[bits >>  0 & 0b111111],
		};
	};

    std::string output;
    output.reserve((input.size() / 3 + 2) * 4);

    for (uint64_t i = 0; i < input.size() / 3; i++) {
        std::ranges::copy(encodeBlock(input[i*3+0], input[i*3+1], input[i*3+2]), std::back_inserter(output));
    }
    if (input.size() % 3 == 2) {
        const auto encodedChars = encodeBlock(input.last(2)[0], input.last(2)[1], static_cast<std::byte>(0));
        output.push_back(encodedChars[0]);
        output.push_back(encodedChars[1]);
        output.push_back(encodedChars[2]);
        output.push_back('=');
    } else if (input.size() % 3 == 1) {
        const auto encodedChars = encodeBlock(input.back(), std::byte{0}, std::byte{0});
    	output.push_back(encodedChars[0]);
    	output.push_back(encodedChars[1]);
    	output.push_back('=');
    	output.push_back('=');
    }

    return output;
}

} // namespace

int main(int argc, const char* const argv[]) {
#if defined(_WIN32)
	SetConsoleOutputCP(CP_UTF8); // Set up console to show UTF-8 characters
	setvbuf(stdout, nullptr, _IOFBF, 1000); // Enable buffering so VS terminal won't chop up UTF-8 byte sequences
#endif

	argparse::ArgumentParser cli{PROJECT_NAME, PROJECT_VERSION, argparse::default_arguments::help};

#ifdef _WIN32
	// Add the Windows-specific ones because why not
	cli.set_prefix_chars("-/");
	cli.set_assign_chars("=:");
#endif

	//region General Arguments

	std::string mode;
	cli
		.add_argument("mode")
		.metavar("MODE")
		.help("The mode to run the program in. This determines what arguments are processed. Valid options:"
		      R"( "create", "edit", "extract", and "info". "convert" is also permissible and is an alias of "create")"
		      " for vtex2 compatibility.")
		.choices("create", "convert", "edit", "extract", "info")
		.required()
		.store_into(mode);

	std::vector<std::string> inputPaths;
	cli
		.add_argument("-i", "--input")
		.metavar("PATH")
		.help("The paths to the input files or directories.")
		.nargs(argparse::nargs_pattern::at_least_one)
		.store_into(inputPaths);

	std::string specificInputPath;
	cli
		.add_argument("path")
		.metavar("PATH")
		.help("The path to the input file or directory.")
		.nargs(argparse::nargs_pattern::optional)
		.default_value(specificInputPath).store_into(specificInputPath);

	std::string outputPath;
	cli
		.add_argument("-o", "--output")
		.metavar("PATH")
		.help("The path to the output file (if the current mode outputs a file), or the parent directory of the output"
		      "files if multiple input paths are specified. Ignored if the input path is a directory.")
		.store_into(outputPath);

	bool overwrite;
	cli
		.add_argument("-y", "--yes")
		.help("Automatically say yes to any prompts. Enabled by default if no TTY is detected.")
		.flag()
		.store_into(overwrite);

	bool noOverwrite;
	cli
		.add_argument("--no")
		.help("Automatically say no to any prompts. Overrides --yes.")
		.flag()
		.store_into(noOverwrite);

	bool quiet;
	cli
		.add_argument("--quiet")
		.help("Don't print anything to stdout or stderr (assuming program arguments are parsed successfully)."
		      " Enabled by default if no TTY is detected.")
		.flag()
		.store_into(quiet);

	bool verbose;
	cli
		.add_argument("--verbose")
		.help("Allow printing to stdout or stderr, even when no TTY is detected (assuming program arguments"
		      " are parsed successfully).")
		.flag()
		.store_into(verbose);

	bool noRecurse;
	cli
		.add_argument("--no-recurse")
		.help("If the input path is a directory, do not enter subdirectories when scanning for files.")
		.flag()
		.store_into(noRecurse);

	bool noPrettyFormatting;
	cli
		.add_argument("--no-pretty-formatting")
		.help("Disables printing ANSI color codes and emojis. Pretty formatting is disabled by default"
		      " if no TTY is detected.")
		.flag()
		.store_into(noPrettyFormatting);

	bool noTTY;
	cli
		.add_argument("--no-tty")
		.help("Override TTY detection if the automatic detection does not work, or for developer testing."
		      " Analogous to \"--yes --quiet --no-pretty-formatting\".")
		.flag()
		.store_into(noTTY);

	//endregion

	//region Program Info

	static constexpr std::string_view PROGRAM_DETAILS{
		"Program details:\n\n"
		PROJECT_NAME_PRETTY " — version v" PROJECT_VERSION " — created by " PROJECT_ORGANIZATION_NAME " — licensed under MIT\n\n"
		"See the project README for more information.\n\n"
		"Want to report a bug or request a feature? Make an issue at " PROJECT_GITHUB_URL "/issues"
	};

	cli.add_epilog(std::string{PROGRAM_DETAILS});

	//endregion

	try {
		cli.parse_args(argc, argv);

		if (noTTY || !::runningInTTY()) {
			overwrite = true;
			quiet = true;
			noPrettyFormatting = true;
		}

		if (quiet && !verbose) {
			tfout_t::QUIET = true;
			tferr_t::QUIET = true;
		} else {
			quiet = false;
		}

		// Pretty formatting colors
		std::string_view END;
		std::string_view RED;
		std::string_view GREEN;
		std::string_view CYAN;
		std::string_view BOLD;
		if (!noPrettyFormatting) {
			END   = "\033[0m";
			RED   = "\033[0;31m";
			GREEN = "\033[0;32m";
			CYAN  = "\033[0;36m";
			BOLD  = "\033[1m";

#ifdef _WIN32
			// Enable ANSI color codes on older Windows builds
			HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			DWORD dwMode = 0;
			GetConsoleMode(hStdOut, &dwMode);
			dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hStdOut, dwMode);
#endif
		}

		// Fix up input/output paths
		if (!specificInputPath.empty()) {
			inputPaths.push_back(specificInputPath);
		}
		if (inputPaths.empty()) {
			throw std::invalid_argument{"No input path(s) provided!"};
		}
		if (inputPaths.size() > 1 && !outputPath.empty() && !std::filesystem::is_directory(outputPath)) {
			throw std::invalid_argument{"Output path must be a directory when manually specified and given multiple inputs!"};
		}

		static constexpr auto DIR_OPTIONS = std::filesystem::directory_options::follow_directory_symlink | std::filesystem::directory_options::skip_permission_denied;

		const auto checkFileDoesntExist = [&](const std::string& currentOutputPath, bool& shouldRet) -> int {
			shouldRet = true;
			const bool exists = std::filesystem::exists(currentOutputPath);
			if (exists && !std::filesystem::is_regular_file(currentOutputPath)) {
				tferr << "Output path at " << BOLD << currentOutputPath << END << " must not be a directory!" << tfendl;
				return EXIT_FAILURE;
			}
			if (exists && noOverwrite) {
				tfout << "Output file at " << BOLD << currentOutputPath << END << " already exists, leaving unmodified." << tfendl;
				return EXIT_SUCCESS;
			}
			if (exists && !overwrite) {
				std::string in;
				while (in.empty() || (!in.starts_with('y') && !in.starts_with('Y') && !in.starts_with('n') && !in.starts_with('N'))) {
					tfout << "Output file at " << BOLD << currentOutputPath << END << " already exists.\nOverwrite? (" << RED << 'y' << END << '/' << GREEN << 'N' << END << ") ";
					std::cin >> in;
				}
				if (in.empty() || in.starts_with('n') || in.starts_with('N')) {
					tfout << "Output file at " << BOLD << currentOutputPath << END << " already exists, leaving unmodified." << tfendl;
					return EXIT_SUCCESS;
				}
			} else if (exists) {
				tfout << "Output file at " << BOLD << currentOutputPath << END << " already exists, overwriting..." << tfendl;
			}
			shouldRet = false;
			return EXIT_SUCCESS;
		};

		// todo: process modes here
	} catch (const std::exception& e) {
		if (argc > 1) {
			std::cerr << e.what() << '\n' << std::endl;
			std::cerr << "Run " << argv[0] << " with no arguments for usage information." << '\n' << std::endl;
		} else {
			std::cout << cli << std::endl;
		}
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
