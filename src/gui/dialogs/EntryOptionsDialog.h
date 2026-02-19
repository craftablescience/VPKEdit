#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <tuple>

#include <QDialog>
#include <vpkpp/Options.h>

#include "PackFileOptionsDialog.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;

class EntryOptionsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath, PackFileOptionsShowForTypeFlags flags, vpkpp::EntryOptions options, QWidget* parent = nullptr);

	[[nodiscard]] vpkpp::EntryOptions getEntryOptions() const;

	static std::optional<std::tuple<QString, vpkpp::EntryOptions>> getEntryOptions(bool edit, bool isDir, const QString& prefilledPath, PackFileOptionsShowForTypeFlags flags, vpkpp::EntryOptions options, QWidget* parent = nullptr);

private:
	QLineEdit* path;

	// BSP/ZIP
	QComboBox* compressionType;
	QSpinBox* compressionStrength;

	// VPK
	QCheckBox* useArchiveVPK;
	QSpinBox*  preloadBytes;
};
