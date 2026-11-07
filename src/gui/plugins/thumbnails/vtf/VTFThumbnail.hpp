#pragma once

#include "../VThumbnailPlugin.hpp"

class VTFThumbnail final : public VThumbnailPlugin_V6_0 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID VThumbnailPlugin_V6_0_iid FILE "VTFThumbnail.json");
	Q_INTERFACES(VThumbnailPlugin_V6_0);

public:
	void initPlugin(VWindowAccess_V6_0*) override;

	[[nodiscard]] const QSet<QString>& getFileExtensions() const override;

	[[nodiscard]] int getPriority() const override;

	[[nodiscard]] QPixmap getThumbnail(const QString&, const quint8* dataPtr, quint64 dataLen) override;
};
