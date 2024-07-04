#pragma once

#include <cstdint>
#include <optional>
#include <tuple>

#include <QDialog>
#include <vpkpp/Options.h>
#include <vpkpp/PackFileType.h>

class QCheckBox;
class QLineEdit;
class QSpinBox;

class EntryOptionsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath, vpkpp::PackFileType type, vpkpp::EntryOptions options, QWidget* parent = nullptr);

	[[nodiscard]] vpkpp::EntryOptions getEntryOptions() const;

	static std::optional<std::tuple<QString, vpkpp::EntryOptions>> getEntryOptions(bool edit, bool isDir, const QString& prefilledPath, vpkpp::PackFileType type, vpkpp::EntryOptions options, QWidget* parent = nullptr);

private:
	QLineEdit* path;

	// VPK
	QCheckBox* useArchiveVPK;
	QSpinBox*  preloadBytes;
};
