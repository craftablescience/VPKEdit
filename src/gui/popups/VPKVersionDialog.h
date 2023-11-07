#pragma once

#include <optional>
#include <tuple>

#include <QDialog>

class QComboBox;

class VPKVersionDialog : public QDialog {
    Q_OBJECT;

public:
    explicit VPKVersionDialog(bool exists, std::uint32_t startVersion = 2, QWidget* parent = nullptr);

    static std::optional<std::tuple<std::uint32_t>> getVPKVersionOptions(bool exists, std::uint32_t startVersion = 2, QWidget* parent = nullptr);

private:
    QComboBox* version;
};
