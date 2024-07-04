#pragma once

#include <optional>
#include <tuple>

#include <QDialog>
#include <vpkpp/Options.h>

class QCheckBox;
class QComboBox;
class QSpinBox;

class NewVPKOptionsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit NewVPKOptionsDialog(bool fromDirectory, vpkpp::PackFileOptions options, bool singleFile, QWidget* parent = nullptr);

	[[nodiscard]] vpkpp::PackFileOptions getPackFileOptions() const;

	static std::optional<std::tuple<vpkpp::PackFileOptions, bool>> getNewVPKOptions(bool fromDirectory, vpkpp::PackFileOptions options, bool singleFile, QWidget* parent = nullptr);

private:
	QComboBox* version;
	QCheckBox* singleFile;
	QSpinBox*  preferredChunkSize;
	QCheckBox* generateMD5Entries;
};
