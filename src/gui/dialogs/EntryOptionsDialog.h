#pragma once

#include <cstdint>
#include <optional>
#include <tuple>

#include <QDialog>
#include <vpkedit/Options.h>
#include <vpkedit/PackFileType.h>

class QCheckBox;
class QLineEdit;
class QSpinBox;

class EntryOptionsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath = QString(), vpkedit::PackFileType type = vpkedit::PackFileType::GENERIC, vpkedit::EntryOptions options = {}, QWidget* parent = nullptr);

	[[nodiscard]] vpkedit::EntryOptions getEntryOptions() const;

	static std::optional<std::tuple<QString, vpkedit::EntryOptions>> getEntryOptions(bool edit, bool isDir, const QString& prefilledPath = QString(), vpkedit::PackFileType type = vpkedit::PackFileType::GENERIC, vpkedit::EntryOptions options = {}, QWidget* parent = nullptr);

private:
	QLineEdit* path;

	// VPK
	QCheckBox* useArchiveVPK;
	QSpinBox* preloadBytes;

	// ZIP/BSP
	QCheckBox* useCompression;
};
