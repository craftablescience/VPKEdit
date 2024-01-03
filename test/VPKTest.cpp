#include <gtest/gtest.h>

#include <vpkedit/VPK.h>

#include <fstream>
#include <string_view>
#include <vector>

// These tests will need Portal 2 installed on your main drive
// Modify this on Linux (I'm the only one running these tests right now)
#define USERNAME "craftablescience"

constexpr auto PORTAL2_PAK_PATH =
#ifdef _WIN32
    R"(C:\Program Files (x86)\Steam\steamapps\common\Portal 2\portal2\pak01_dir.vpk)";
#else // __linux__
    "/home/" USERNAME "/.steam/steam/steamapps/common/Portal 2/portal2/pak01_dir.vpk";
#endif

using namespace vpkedit;

TEST(VPK, read) {
    auto vpk = VPK::open(PORTAL2_PAK_PATH);
    ASSERT_TRUE(vpk);

    for (const auto& [directory, files] : vpk->getBakedEntries()) {
        for (const auto& file : files) {
            // Terminal explosion
            std::cout << directory << '/' << file.filename << '\n';
        }
    }
}

TEST(VPK, readContents) {
    // Ditto
    auto vpk = VPK::open(PORTAL2_PAK_PATH);
    ASSERT_TRUE(vpk);

    auto cableVMT = vpk->findEntry("materials/cable/cable.vmt");
    ASSERT_TRUE(cableVMT);

    ASSERT_EQ(cableVMT->length, 46);

    std::string_view expectedContents = "SplineRope\r\n{\r\n$basetexture \"cable\\black\"\r\n}\r\n";
    auto actualContents = vpk->readTextEntry(*cableVMT);
    ASSERT_TRUE(actualContents);
    ASSERT_STREQ(actualContents->c_str(), expectedContents.data());
}
