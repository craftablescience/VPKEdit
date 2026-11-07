// ReSharper disable CppUnusedIncludeDirective

#pragma once

#include <QPixmap>
#include <QSet>
#include <QString>

#include "../VWindowAccess.hpp"

class VThumbnailPlugin_V6_0 : public QObject {
	Q_OBJECT;

public:
	virtual void initPlugin(VWindowAccess_V6_0* windowAccess) = 0;

	[[nodiscard]] virtual const QSet<QString>& getFileExtensions() const = 0;

	[[nodiscard]] virtual int getPriority() const = 0;

	// Sorry, QSpan is too new
	[[nodiscard]] virtual QPixmap getThumbnail(const QString& path, const quint8* dataPtr, quint64 dataLen) = 0;
};

#define VThumbnailPlugin_V6_0_iid "app.vpkedit.VThumbnailPlugin/6.0"
Q_DECLARE_INTERFACE(VThumbnailPlugin_V6_0, VThumbnailPlugin_V6_0_iid)
