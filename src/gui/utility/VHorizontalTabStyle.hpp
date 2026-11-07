#pragma once

#include <QProxyStyle>

class QPainter;
class QStyleOption;
class QWidget;

class VHorizontalTabStyle : public QProxyStyle {
public:
	[[nodiscard]] QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;

	void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
};
