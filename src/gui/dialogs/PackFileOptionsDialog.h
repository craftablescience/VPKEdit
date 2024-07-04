#pragma once

#include <optional>
#include <tuple>

#include <QDialog>
#include <vpkpp/Options.h>
#include <vpkpp/PackFileType.h>

class QCheckBox;
class QComboBox;

class PackFileOptionsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit PackFileOptionsDialog(vpkpp::PackFileType type, vpkpp::PackFileOptions options_, QWidget* parent = nullptr);

	[[nodiscard]] vpkpp::PackFileOptions getPackFileOptions();

	static std::optional<vpkpp::PackFileOptions> getPackFileOptions(vpkpp::PackFileType type, vpkpp::PackFileOptions options, QWidget* parent = nullptr);

private:
	vpkpp::PackFileOptions options;

	QComboBox* vpk_version;
#ifdef VPKEDIT_ZIP_COMPRESSION
	QCheckBox* zip_useCompression;
#endif
};
