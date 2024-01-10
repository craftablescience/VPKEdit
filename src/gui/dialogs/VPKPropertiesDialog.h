#pragma once

#include <optional>
#include <tuple>

#include <QDialog>

class QCheckBox;
class QComboBox;

class VPKPropertiesDialog : public QDialog {
    Q_OBJECT;

public:
    explicit VPKPropertiesDialog(bool exists, std::uint32_t startVersion = 2, bool singleFile = false, QWidget* parent = nullptr);

    static std::optional<std::tuple<std::uint32_t, bool>> getVPKProperties(bool exists, std::uint32_t startVersion = 2, bool singleFile = false, QWidget* parent = nullptr);

private:
    QComboBox* version;
	QCheckBox* singleFile;
};
