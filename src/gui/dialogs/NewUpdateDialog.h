#pragma once

#include <QMessageBox>

class NewUpdateDialog : public QMessageBox {
    Q_OBJECT;

public:
    explicit NewUpdateDialog(const QString& releaseLink, const QString& version, const QString& details, QWidget* parent = nullptr);

    static void getNewUpdatePrompt(const QString& releaseLink, const QString& version, const QString& details, QWidget* parent = nullptr);
};
