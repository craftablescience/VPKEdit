#pragma once

#include <QLabel>

class InfoPreview : public QWidget {
    Q_OBJECT;

public:
    InfoPreview(const QPixmap& icon, const QString& text, QWidget* parent = nullptr);

	explicit InfoPreview(QWidget* parent = nullptr);

	void setData(const QPixmap& icon, const QString& text);

protected:
	QLabel* image;
	QLabel* error;
};
