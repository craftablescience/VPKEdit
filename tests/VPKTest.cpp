#include <gtest/gtest.h>

#include <libvpktool/VPK.h>

#include <fstream>
#include <string_view>
#include <vector>

using namespace vpktool;

TEST(VPK, read) {
    // Going to assume this is my computer lol
    // Install Portal 2 on your Windows C drive for this to work
    auto vpk = VPK::open(R"(C:\Program Files (x86)\Steam\steamapps\common\Portal 2\portal2\pak01_dir.vpk)");
    ASSERT_TRUE(vpk);

    for (const auto& [directory, files] : vpk->getEntries()) {
        for (const auto& file : files) {
            // Terminal explosion
            std::cout << directory << '/' << file.filename << '\n';
        }
    }
}

TEST(VPK, readContents) {
    // Ditto
    auto vpk = VPK::open(R"(C:\Program Files (x86)\Steam\steamapps\common\Portal 2\portal2\pak01_dir.vpk)");
    ASSERT_TRUE(vpk);

    auto cableVMT = vpk->findEntry("materials/cable/cable.vmt");
    ASSERT_TRUE(cableVMT);

    ASSERT_EQ(cableVMT->length, 46);

    std::string_view expectedContents = "SplineRope\r\n{\r\n$basetexture \"cable\\black\"\r\n}\r\n";
    auto actualContents = vpk->readTextEntry(*cableVMT);
    ASSERT_STREQ(actualContents.c_str(), expectedContents.data());
}
