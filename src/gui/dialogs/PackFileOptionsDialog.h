#pragma once

#include <optional>
#include <tuple>

#include <QDialog>
#include <vpkedit/Options.h>
#include <vpkedit/PackFileType.h>

class QComboBox;

class PackFileOptionsDialog : public QDialog {
    Q_OBJECT;

public:
    explicit PackFileOptionsDialog(vpkedit::PackFileType type, vpkedit::PackFileOptions options_, QWidget* parent = nullptr);

	[[nodiscard]] vpkedit::PackFileOptions getPackFileOptions();

    static std::optional<vpkedit::PackFileOptions> getPackFileOptions(vpkedit::PackFileType type, vpkedit::PackFileOptions options, QWidget* parent = nullptr);

private:
	vpkedit::PackFileOptions options;

	QComboBox* vpk_version;
};
