#include <gtest/gtest.h>

#include <vpktool/VPK.h>

#include <iostream>

TEST(VPK, read) {
    // Going to assume this is my computer lol
    // Install Portal 2 on your Windows C drive for this to work
    vpktool::VPK vpk{R"(C:\Program Files (x86)\Steam\steamapps\common\Portal 2\portal2\pak01_dir.vpk)"};
    ASSERT_TRUE(vpk);

    for (const auto& [extension, files] : vpk.entries) {
        for (const auto& file : files) {
            // Terminal explosion
            std::cout << file.getFullPath() << '\n';
        }
    }
}
