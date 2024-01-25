#pragma once

#include <cstdint>
#include <optional>
#include <tuple>

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QSpinBox;

class EntryOptionsDialog : public QDialog {
    Q_OBJECT;

public:
    explicit EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath = QString(), bool isVPK = true, bool prefilledUseArchiveVPK = false, std::uint32_t prefilledPreloadBytes = 0, QWidget* parent = nullptr);

    static std::optional<std::tuple<QString, bool, std::uint32_t>> getEntryOptions(bool edit, bool isDir, const QString& prefilledPath = QString(), bool isVPK = true, bool prefilledUseArchiveVPK = false, std::uint32_t prefilledPreloadBytes = 0, QWidget* parent = nullptr);

private:
    QLineEdit* path;
    QCheckBox* useArchiveVPK;
    QSpinBox* preloadBytes;
};
