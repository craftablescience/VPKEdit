#pragma once

#include <QIcon>
#include <QSet>
#include <QString>

#include "../IVPKEditWindowAccess.h"

class QWidget;

class IVPKEditPreviewPlugin_V1_3 : public QObject {
	Q_OBJECT;

public:
	virtual void initPlugin(IVPKEditWindowAccess_V3* windowAccess) = 0;

	virtual void initPreview(QWidget* parent) = 0;

	[[nodiscard]] virtual QWidget* getPreview() const = 0;

	[[nodiscard]] virtual const QSet<QString>& getPreviewExtensions() const = 0;

	[[nodiscard]] virtual QIcon getIcon() const = 0;

	enum Error {
		ERROR_SHOWED_THIS_PREVIEW  = 0,
		ERROR_SHOWED_OTHER_PREVIEW = 1,
		ERROR_SHOWED_NO_PREVIEW    = 2,
	};

	// Sorry, QSpan is too new
	virtual Error setData(const QString& path, const quint8* dataPtr, quint64 length) = 0;

signals:
	void showTextPreview(const QString& text, const QString& extension);

	bool showInfoPreview(const QPixmap& icon, const QString& text);

	void showGenericErrorPreview(const QString& text);
};

#define IVPKEditPreviewPlugin_V1_3_iid "info.craftablescience.vpkedit.IPreviewPlugin/1.3"
Q_DECLARE_INTERFACE(IVPKEditPreviewPlugin_V1_3, IVPKEditPreviewPlugin_V1_3_iid)
