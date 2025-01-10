#pragma once

#include "Folder.h"

class SingleFile : public Folder {
public:
	/// Open a previewable file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, const EntryCallback& callback = nullptr);

	static constexpr inline std::string_view GUID = "13DBA85EDC294BA6A6979EAF071246DE";

	[[nodiscard]] constexpr std::string_view getGUID() const override {
		return SingleFile::GUID;
	}

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const std::string& path_) const override;

protected:
	using Folder::Folder;

private:
	// Audio
	VPKPP_REGISTER_PACKFILE_OPEN(".wav", &SingleFile::open);

	// DMX
	VPKPP_REGISTER_PACKFILE_OPEN(".dmx", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".movieobjects", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".sfm", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".sfm_settings", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".sfm_session", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".sfm_trackgroup", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".pcf", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gui", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".schema", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".preset", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".facial_animation", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".model", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".ved", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mks", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vmks", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_preprocess", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_root", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_model", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_anim", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_physics", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_hitbox", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_materialgroup", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_keyvalues", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_eyes", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".mp_bonemask", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".tex", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".world", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".worldnode", &SingleFile::open);

	// MDL
	VPKPP_REGISTER_PACKFILE_OPEN(".mdl", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vtx", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vvd", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".phy", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".ani", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vta", &SingleFile::open);

	// Text
	VPKPP_REGISTER_PACKFILE_OPEN(".txt", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".md", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".nut", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gnut", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".lua", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gm", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".py", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".js", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".ts", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".map", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vmf", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vmm", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vmx", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vmt", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vcd", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".fgd", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".qc", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".qci", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".qcx", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".smd", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".kv", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".kv3", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".res", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vdf", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".acf", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".bns", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".zpc", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".zpdata", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vbsp", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".rad", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gam", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gi", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".rc", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".lst", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".cfg", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".ini", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".yml", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".yaml", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".toml", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".json", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".html", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".htm", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".xml", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".css", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".scss", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".sass", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN("authors", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN("credits", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN("license", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN("readme", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gitignore", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gitattributes", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gitmodules", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gd", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gdshader", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".tscn", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".tres", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".import", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".remap", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".pop", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".edt", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".set", &SingleFile::open);

	// Textures
	VPKPP_REGISTER_PACKFILE_OPEN(".tga", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".jpg", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".jpeg", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".jfif", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".png", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".apng", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".webp", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".bmp", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".psd", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".gif", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".hdr", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".exr", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".pic", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".ppm", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".pgm", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".svg", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".ppl", &SingleFile::open);
	VPKPP_REGISTER_PACKFILE_OPEN(".vtf", &SingleFile::open);
};
