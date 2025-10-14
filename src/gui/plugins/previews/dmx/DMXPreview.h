#pragma once

#include "../IVPKEditPreviewPlugin.h"

class QTreeWidget;

class DMXPreview final : public IVPKEditPreviewPlugin_V1_0 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID IVPKEditPreviewPlugin_V1_0_iid FILE "DMXPreview.json");
	Q_INTERFACES(IVPKEditPreviewPlugin_V1_0);

public:
	void initPlugin(IVPKEditWindowAccess_V1*) override;

	void initPreview(QWidget* parent) override;

	[[nodiscard]] QWidget* getPreview() const override;

	[[nodiscard]] QIcon getIcon() const override;

	Error setData(const QString&, const quint8* dataPtr, quint64 length) override;

private:
	QTreeWidget* preview = nullptr;
};
