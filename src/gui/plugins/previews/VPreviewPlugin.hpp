// ReSharper disable CppUnusedIncludeDirective

#pragma once

#include <QPixmap>
#include <QSet>
#include <QString>
#include <QWidget>

#include "../VWindowAccess.hpp"

class QWidget;

class VPreviewPlugin_V6_0 : public QObject {
	Q_OBJECT;

public:
	virtual void initPlugin(VWindowAccess_V6_0* windowAccess) = 0;

	[[nodiscard]] virtual const QSet<QString>& getFileExtensions() const = 0;

	[[nodiscard]] virtual int getPriority() const = 0;

	[[nodiscard]] virtual QWidget* getPreview(QWidget* parent) const = 0;

	// Sorry, QSpan is too new
	[[nodiscard]] virtual bool setData(QWidget* preview, const QString& path, const quint8* dataPtr, quint64 length) = 0;

signals:
	void showErrorPreview(const QString& text);

	bool showInfoPreview(const QPixmap& icon, const QString& text);

	void showTextPreview(const QString& extension, const QString& text);
};

#define VPreviewPlugin_V6_0_iid "app.vpkedit.VPreviewPlugin/6.0"
Q_DECLARE_INTERFACE(VPreviewPlugin_V6_0, VPreviewPlugin_V6_0_iid)
