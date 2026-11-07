#pragma once

#include "../IVPKEditPreviewPlugin.hpp"

class QTreeWidget;

class DMXPreview final : public IVPKEditPreviewPlugin_V6_0 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID IVPKEditPreviewPlugin_V6_0_iid FILE "DMXPreview.json");
	Q_INTERFACES(IVPKEditPreviewPlugin_V6_0);

public:
	void initPlugin(IVPKEditWindowAccess_V6_0*) override;

	[[nodiscard]] const QSet<QString>& getPreviewExtensions() const override;

	[[nodiscard]] int getPriority() const override;

	[[nodiscard]] QWidget* getPreview(QWidget* parent) const override;

	[[nodiscard]] bool setData(QWidget* preview_, const QString&, const quint8* dataPtr, quint64 dataLen) override;
};
