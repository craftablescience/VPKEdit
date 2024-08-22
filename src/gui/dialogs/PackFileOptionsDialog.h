#pragma once

#include <optional>

#include <QDialog>
#include <vpkpp/format/VPK.h>

class QCheckBox;
class QComboBox;
class QSpinBox;

struct PackFileOptions {
	unsigned int vpk_version = 2;
	bool vpk_saveSingleFile = false;
	unsigned int vpk_chunkSize = vpkpp::VPK_DEFAULT_CHUNK_SIZE;
};

class PackFileOptionsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit PackFileOptionsDialog(vpkpp::PackFileType type, bool editing, bool createFromDir, PackFileOptions options, QWidget* parent = nullptr);

	[[nodiscard]] PackFileOptions getPackFileOptions() const;

	static std::optional<PackFileOptions> getForNew(vpkpp::PackFileType type, bool createFromDir, QWidget* parent = nullptr);

	static std::optional<PackFileOptions> getForEdit(vpkpp::PackFileType type, PackFileOptions options, QWidget* parent = nullptr);

private:
	QComboBox* version;
	QCheckBox* singleFile;
	QSpinBox*  preferredChunkSize;
};
