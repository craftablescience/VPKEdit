#pragma once

#include <optional>
#include <tuple>

#include <QDialog>

class QComboBox;

class NewVPKDialog : public QDialog {
    Q_OBJECT;

public:
    explicit NewVPKDialog(QWidget* parent = nullptr);

    static std::optional<std::tuple<int>> getNewVPKOptions(QWidget* parent = nullptr);

private:
    QComboBox* version;
};
