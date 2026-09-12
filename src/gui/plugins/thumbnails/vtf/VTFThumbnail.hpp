#pragma once

#include "../IVPKEditThumbnailPlugin.hpp"

class VTFThumbnail final : public IVPKEditThumbnailPlugin_V6_0 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID IVPKEditThumbnailPlugin_V6_0_iid FILE "VTFThumbnail.json");
	Q_INTERFACES(IVPKEditThumbnailPlugin_V6_0);

public:
	void initPlugin(IVPKEditWindowAccess_V6_0*) override;

	[[nodiscard]] const QSet<QString>& getFileExtensions() const override;

	[[nodiscard]] int getPriority() const override;

	[[nodiscard]] QPixmap getThumbnail(const QString&, const quint8* dataPtr, quint64 dataLen) override;
};
