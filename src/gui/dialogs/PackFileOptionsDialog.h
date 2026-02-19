#pragma once

#include <optional>

#include <QDialog>
#include <sourcepp/Macros.h>
#include <vpkpp/format/VPK.h>
#include <vpkpp/Options.h>

class QCheckBox;
class QComboBox;
class QSpinBox;

enum class PackFileOptionsShowForTypeFlags : uint64_t {
	ORDINARY = 0,
	BSP = 1 << 0,
	FPX = 1 << 1,
	VPK = 1 << 2,
	ZIP = 1 << 3,
};
SOURCEPP_BITFLAGS_ENUM(PackFileOptionsShowForTypeFlags)

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
	explicit PackFileOptionsDialog(PackFileOptionsShowForTypeFlags flags, bool editing, bool createFromDir, PackFileOptions options, QWidget* parent = nullptr);

	[[nodiscard]] PackFileOptions getPackFileOptions() const;

	static std::optional<PackFileOptions> getForNew(PackFileOptionsShowForTypeFlags flags, bool createFromDir, QWidget* parent = nullptr);

	static std::optional<PackFileOptions> getForEdit(PackFileOptionsShowForTypeFlags flags, PackFileOptions options, QWidget* parent = nullptr);

private:
	QComboBox* compressionType;
	QSpinBox* compressionStrength;
	QComboBox* version;
	QCheckBox* singleFile;
	QSpinBox*  preferredChunkSize;
};
