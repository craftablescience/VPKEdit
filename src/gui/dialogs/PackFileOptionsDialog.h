#pragma once

#include <optional>

#include <QDialog>
#include <vpkpp/format/VPK.h>
#include <vpkpp/Options.h>

class QCheckBox;
class QComboBox;
class QSpinBox;

struct PackFileOptions {
	vpkpp::EntryCompressionType compressionType = vpkpp::EntryCompressionType::NO_COMPRESS;
	short compressionStrength = 5;
	unsigned int vpk_version = 2;
	bool vpk_saveSingleFile = false;
	unsigned int vpk_chunkSize = vpkpp::VPK_DEFAULT_CHUNK_SIZE;
};

class PackFileOptionsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit PackFileOptionsDialog(std::string_view typeGUID, bool editing, bool createFromDir, PackFileOptions options, QWidget* parent = nullptr);

	[[nodiscard]] PackFileOptions getPackFileOptions() const;

	static std::optional<PackFileOptions> getForNew(std::string_view typeGUID, bool createFromDir, QWidget* parent = nullptr);

	static std::optional<PackFileOptions> getForEdit(std::string_view typeGUID, PackFileOptions options, QWidget* parent = nullptr);

private:
	QComboBox* compressionType;
	QSpinBox* compressionStrength;
	QComboBox* version;
	QCheckBox* singleFile;
	QSpinBox*  preferredChunkSize;
};
