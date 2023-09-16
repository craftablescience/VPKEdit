#pragma once

#include <optional>
#include <tuple>

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QSpinBox;

class NewEntryDialog : public QDialog {
    Q_OBJECT;

public:
    explicit NewEntryDialog(QWidget* parent = nullptr, const QString& prefilledPath = QString());

    static std::optional<std::tuple<QString, bool, int>> getNewEntryOptions(QWidget* parent = nullptr, const QString& prefilledPath = QString());

private:
    QLineEdit* path;
    QCheckBox* useArchiveVPK;
    QSpinBox* preloadBytes;
};
