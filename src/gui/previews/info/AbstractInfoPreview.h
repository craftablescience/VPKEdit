#pragma once

#include <QLabel>

class AbstractInfoPreview : public QWidget {
    Q_OBJECT;

public:
    explicit AbstractInfoPreview(const QPixmap& icon, const QString& text, QWidget* parent = nullptr);

protected:
	QLabel* image;
	QLabel* error;
};
