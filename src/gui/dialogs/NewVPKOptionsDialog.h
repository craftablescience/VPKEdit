#pragma once

#include <optional>
#include <tuple>

#include <QDialog>
#include <vpkedit/Options.h>

class QCheckBox;
class QComboBox;
class QSpinBox;

class NewVPKOptionsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit NewVPKOptionsDialog(bool fromDirectory, vpkedit::PackFileOptions options, bool singleFile, QWidget* parent = nullptr);

	[[nodiscard]] vpkedit::PackFileOptions getPackFileOptions() const;

	static std::optional<std::tuple<vpkedit::PackFileOptions, bool>> getNewVPKOptions(bool fromDirectory, vpkedit::PackFileOptions options, bool singleFile, QWidget* parent = nullptr);

private:
	QComboBox* version;
	QCheckBox* singleFile;
	QSpinBox*  preferredChunkSize;
	QCheckBox* generateMD5Entries;
};
