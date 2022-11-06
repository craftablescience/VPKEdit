#include <gtest/gtest.h>

#include <vpktool/VPK.h>

#include <iostream>
#include <fstream>
#include <vector>

std::vector<unsigned char> readFileBytes(const std::string& filename) {
    std::ifstream file{filename, std::ios::binary};
    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> fileData;
    fileData.reserve(fileSize);
    file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
    return fileData;
}

TEST(VPK, read) {
    // Going to assume this is my computer lol
    // Install Portal 2 on your Windows C drive for this to work
    auto bytes = readFileBytes(R"(C:\Program Files (x86)\Steam\steamapps\common\Portal 2\portal2\pak01_dir.vpk)");
    vpktool::VPK vpk{bytes.data(), bytes.size()};
    ASSERT_TRUE(vpk);

    for (const auto& [extension, files] : vpk.entries) {
        for (const auto& file : files) {
            // Terminal explosion
            std::cout << file.getFullPath() << '\n';
        }
    }
}
