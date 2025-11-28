#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include <QDialog>

#include "../IVPKEditPreviewPlugin.h"

class QComboBox;
class QLineEdit;

constexpr std::string_view STR_VICE_CODE_INDEX = "vice_dialog_code_index";
constexpr std::string_view STR_VICE_CODE_VALUE = "vice_dialog_code_value";

class VICEDialog : public QDialog {
	Q_OBJECT;

public:
	VICEDialog(QString path_, bool encrypt, IVPKEditWindowAccess_V3* windowAccess_, QWidget* parent = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> getData() const;

	static std::optional<std::vector<std::byte>> encrypt(const QString& path, IVPKEditWindowAccess_V3* windowAccess, QWidget* parent = nullptr);

	static std::optional<std::vector<std::byte>> decrypt(const QString& path, IVPKEditWindowAccess_V3* windowAccess, QWidget* parent = nullptr);

private:
	QComboBox* codes;
	QLineEdit* customCode;

	QString path;
	bool encrypting;
	IVPKEditWindowAccess_V3* windowAccess;

	static QList<std::pair<QString, std::string_view>> CODES;
};

class VICEPreview final : public IVPKEditPreviewPlugin_V1_3 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID IVPKEditPreviewPlugin_V1_3_iid FILE "VICEPreview.json");
	Q_INTERFACES(IVPKEditPreviewPlugin_V1_3);

public:
	void initPlugin(IVPKEditWindowAccess_V3* windowAccess_) override;

	void initPreview(QWidget* parent) override;

	[[nodiscard]] QWidget* getPreview() const override;

	[[nodiscard]] const QSet<QString>& getPreviewExtensions() const override;

	[[nodiscard]] QIcon getIcon() const override;

	Error setData(const QString& path, const quint8*, quint64) override;

private:
	QWidget* preview = nullptr;

	QString selectedPath;
	IVPKEditWindowAccess_V3* windowAccess = nullptr;
};
