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
    explicit NewEntryDialog(bool isDir, const QString& prefilledPath = QString(), QWidget* parent = nullptr);

    static std::optional<std::tuple<QString, bool, int>> getNewEntryOptions(bool isDir, const QString& prefilledPath = QString(), QWidget* parent = nullptr);

private:
    QLineEdit* path;
    QCheckBox* useArchiveVPK;
    QSpinBox* preloadBytes;
};
