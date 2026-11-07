#pragma once

#include "../VPreviewPlugin.hpp"

class QTreeWidget;

class DMXPreview final : public VPreviewPlugin_V6_0 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID VPreviewPlugin_V6_0_iid FILE "DMXPreview.json");
	Q_INTERFACES(VPreviewPlugin_V6_0);

public:
	void initPlugin(VWindowAccess_V6_0*) override;

	[[nodiscard]] const QSet<QString>& getPreviewExtensions() const override;

	[[nodiscard]] int getPriority() const override;

	[[nodiscard]] QWidget* getPreview(QWidget* parent) const override;

	[[nodiscard]] bool setData(QWidget* preview_, const QString&, const quint8* dataPtr, quint64 dataLen) override;
};
