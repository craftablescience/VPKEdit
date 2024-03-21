#pragma once

#include <QTreeWidget>

class FileViewer;

class DMXPreview : public QTreeWidget {
	Q_OBJECT;

public:
	static inline const QStringList EXTENSIONS {
		".dmx", ".movieobjects", ".sfm", ".sfm_settings", ".sfm_session", ".sfm_trackgroup", ".pcf", ".gui",
		".schema", ".preset", ".facial_animation", ".model", ".ved", ".mks", ".vmks", ".mp_preprocess",
		".mp_root", ".mp_model", ".mp_anim", ".mp_physics", ".mp_hitbox", ".mp_materialgroup", ".mp_keyvalues",
		".mp_eyes", ".mp_bonemask", ".tex", ".world", ".worldnode",
	};

	explicit DMXPreview(FileViewer* fileViewer_, QWidget* parent = nullptr);

	void setData(const std::vector<std::byte>& data);

private:
	FileViewer* fileViewer;
};
