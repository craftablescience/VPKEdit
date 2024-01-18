#pragma once

#include <QDialog>

class NewUpdateDialog : public QDialog {
    Q_OBJECT;

public:
    explicit NewUpdateDialog(const QString& releaseLink, const QString& version, QWidget* parent = nullptr);

    static void getNewUpdatePrompt(const QString& releaseLink, const QString& version, QWidget* parent = nullptr);
};
