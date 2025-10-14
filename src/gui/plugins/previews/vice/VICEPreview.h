#pragma once

#include "../IVPKEditPreviewPlugin.h"

class QLabel;

class VICEPreview final : public IVPKEditPreviewPlugin_V1_1 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID IVPKEditPreviewPlugin_V1_1_iid FILE "VICEPreview.json");
	Q_INTERFACES(IVPKEditPreviewPlugin_V1_1);

public:
	void initPlugin(IVPKEditWindowAccess_V2*) override;

	void initPreview(QWidget* parent) override;

	[[nodiscard]] QWidget* getPreview() const override;

	[[nodiscard]] QIcon getIcon() const override;

	Error setData(const QString&, const quint8* dataPtr, quint64 length) override;

private:
	QLabel* preview = nullptr;
};
