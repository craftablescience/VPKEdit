#pragma once

#include "../IVPKEditPreviewPlugin.h"

class QTreeWidget;

class DMXPreview final : public IVPKEditPreviewPlugin {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID IVPKEditPreviewPlugin_iid FILE "DMXPreview.json");
	Q_INTERFACES(IVPKEditPreviewPlugin);

public:
	void initPreview(QWidget* parent) override;

	QWidget* getPreview() override;

	Error setData(const QString&, const quint8* dataPtr, quint64 length) override;

private:
	QTreeWidget* preview = nullptr;
};
