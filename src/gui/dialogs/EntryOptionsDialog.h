#pragma once

#include <optional>
#include <tuple>

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QSpinBox;

class EntryOptionsDialog : public QDialog {
    Q_OBJECT;

public:
    explicit EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath = QString(), bool prefilledUseDirVPK = true, int prefilledPreloadBytes = 0, QWidget* parent = nullptr);

    static std::optional<std::tuple<QString, bool, int>> getEntryOptions(bool edit, bool isDir, const QString& prefilledPath = QString(), bool prefilledUseDirVPK = true, int prefilledPreloadBytes = 0, QWidget* parent = nullptr);

private:
    QLineEdit* path;
    QCheckBox* useDirVPK;
    QSpinBox* preloadBytes;
};
