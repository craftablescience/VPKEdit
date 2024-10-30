#include "Tree.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

using namespace sourcepp;
using namespace vpkpp;

namespace {

constexpr std::string_view CLR_BG_MAGENTA_START = "\033[1;45m";
constexpr std::string_view CLR_FG_BOLD_RED_START = "\033[1;31m";
constexpr std::string_view CLR_FG_GREEN_START = "\033[32m";
constexpr std::string_view CLR_FG_CYAN_START = "\033[36m";
constexpr std::string_view CLR_END = "\033[0m";

struct TreeNode {
	std::string name;
	const Entry* entry = nullptr;
	std::map<std::string, std::unique_ptr<TreeNode>> children;
};

void insertPath(std::unique_ptr<TreeNode>& node, const std::vector<std::string>& parts, const Entry& entry, uint64_t index = 0) {
	if (index == parts.size()) return;

	const auto& part = parts[index];
	if (node->children.find(part) == node->children.end()) {
		node->children[part] = std::unique_ptr<TreeNode>{new TreeNode{part, &entry}};
	}
	::insertPath(node->children[part], parts, entry, index + 1);
}

void printTree(std::unique_ptr<TreeNode>& node, const std::string& prefix = "", bool isLast = true) {
	std::cout << prefix;
	if (node->children.empty() && node->entry) {
		double filesize = node->entry->length;
		std::string sizeExt = "b";
		if (filesize > 1024) {
			filesize /= 1024.0;
			sizeExt = "kb";
		}
		if (filesize > 1024) {
			filesize /= 1024.0;
			sizeExt = "mb";
		}
		if (filesize > 1024) {
			filesize /= 1024.0;
			sizeExt = "gb";
		}

		std::cout << (isLast ? "└─ " : "│  ") << CLR_FG_GREEN_START << node->name << CLR_END << " - " << CLR_FG_BOLD_RED_START;
		if (sizeExt == "b") {
			std::cout << static_cast<uint16_t>(filesize);
		} else {
			std::cout << std::fixed << std::setprecision(2) << filesize;
		}
		std::cout << ' ' << sizeExt << CLR_END << std::endl;
	} else if (!prefix.empty()) {
		std::cout << (isLast ? "└─ " : "├─ ") << CLR_FG_CYAN_START << node->name << CLR_END << std::endl;
	}

	std::string childPrefix = prefix + (isLast ? "   " : "│  ");
	for (auto it = node->children.begin(); it != node->children.end(); ++it) {
		bool lastChild = std::next(it) == node->children.end();
		::printTree(it->second, childPrefix, lastChild);
	}
}

} // namespace

void prettyPrintPackFile(std::unique_ptr<PackFile>& packFile) {
	std::unique_ptr<TreeNode> root{new TreeNode{packFile->getTruncatedFilename()}};

	packFile->runForAllEntries([&root](const std::string& path, const Entry& entry) {
		::insertPath(root, string::split(path, '/'), entry);
	});

	std::cout << CLR_BG_MAGENTA_START << packFile->getTruncatedFilename() << CLR_END << std::endl;
	::printTree(root);
}
