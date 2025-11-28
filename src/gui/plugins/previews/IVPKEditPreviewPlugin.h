#pragma once

#include <QIcon>
#include <QSet>
#include <QString>

#include "../IVPKEditWindowAccess.h"

class QMenu;
class QWidget;

class IVPKEditPreviewPlugin_V1_3 : public QObject {
	Q_OBJECT;

public:
	virtual void initPlugin(IVPKEditWindowAccess_V3* windowAccess) = 0;

	virtual void initPreview(QWidget* parent) = 0;

	[[nodiscard]] virtual QWidget* getPreview() const = 0;

	[[nodiscard]] virtual const QSet<QString>& getPreviewExtensions() const = 0;

	[[nodiscard]] virtual QIcon getIcon() const = 0;

	enum Error : int {
		ERROR_SHOWED_THIS_PREVIEW  = 0,
		ERROR_SHOWED_OTHER_PREVIEW = 1,
		ERROR_SHOWED_NO_PREVIEW    = 2,
	};

	// Sorry, QSpan is too new
	[[nodiscard]] virtual int setData(const QString& path, const quint8* dataPtr, quint64 length) = 0;

	enum ContextMenuType : int {
		CONTEXT_MENU_TYPE_FILE  = 0,
		CONTEXT_MENU_TYPE_DIR   = 1,
		CONTEXT_MENU_TYPE_ROOT  = 2,
		CONTEXT_MENU_TYPE_MIXED = 3,
	};

	virtual void initContextMenu(int contextMenuType, QMenu* contextMenu) = 0;

	virtual void updateContextMenu(int contextMenuType, const QStringList& paths) = 0;

signals:
	void showTextPreview(const QString& text, const QString& extension);

	bool showInfoPreview(const QPixmap& icon, const QString& text);

	void showGenericErrorPreview(const QString& text);
};

#define IVPKEditPreviewPlugin_V1_3_iid "info.craftablescience.vpkedit.IPreviewPlugin/1.3"
Q_DECLARE_INTERFACE(IVPKEditPreviewPlugin_V1_3, IVPKEditPreviewPlugin_V1_3_iid)
