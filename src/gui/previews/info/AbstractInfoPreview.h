#pragma once

#include <QWidget>

class AbstractInfoPreview : public QWidget {
    Q_OBJECT;

public:
    explicit AbstractInfoPreview(const QPixmap& icon, const QString& text, QWidget* parent = nullptr);
};
