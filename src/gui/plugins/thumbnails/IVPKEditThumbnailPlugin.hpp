// ReSharper disable CppUnusedIncludeDirective

#pragma once

#include <QPixmap>
#include <QSet>
#include <QString>

#include "../IVPKEditWindowAccess.hpp"

class IVPKEditThumbnailPlugin_V6_0 : public QObject {
	Q_OBJECT;

public:
	virtual void initPlugin(IVPKEditWindowAccess_V6_0* windowAccess) = 0;

	[[nodiscard]] virtual const QSet<QString>& getFileExtensions() const = 0;

	[[nodiscard]] virtual int getPriority() const = 0;

	// Sorry, QSpan is too new
	[[nodiscard]] virtual QPixmap getThumbnail(const QString& path, const quint8* dataPtr, quint64 dataLen) = 0;
};

#define IVPKEditThumbnailPlugin_V6_0_iid "app.vpkedit.IThumbnailPlugin/6.0"
Q_DECLARE_INTERFACE(IVPKEditThumbnailPlugin_V6_0, IVPKEditThumbnailPlugin_V6_0_iid)
