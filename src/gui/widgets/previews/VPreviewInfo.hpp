#pragma once

#include <QWidget>

class QLabel;

class VPreviewInfo : public QWidget {
	Q_OBJECT;

public:
	VPreviewInfo(const QPixmap& icon, const QString& text, QWidget* parent = nullptr);

	explicit VPreviewInfo(QWidget* parent = nullptr);

	void setIcon(const QPixmap& icon_) const;

	void setText(const QString& text_) const;

protected:
	QLabel* icon;
	QLabel* text;
};
