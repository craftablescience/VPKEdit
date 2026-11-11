#pragma once

#include <memory>

#include <QProgressBar>
#include <sndfile.hh>

#include "../VPreviewPlugin.hpp"

// todo: all the UI
// todo: bundle mp3/ogg/vorbis/flac libraries
// todo: stop playing when moving to different preview

class SDL_AudioStream;

struct VAudioFile {
	QByteArray data;
	sf_count_t head;

	static SF_VIRTUAL_IO SF_VIO;
};

class VAudioPreviewWidget : public QWidget {
	Q_OBJECT;

public:
	explicit VAudioPreviewWidget(QWidget* parent = nullptr);

	~VAudioPreviewWidget() override;

	[[nodiscard]] QString loadAudio(const quint8* dataPtr, quint64 dataLen);

	void seek(sf_count_t frame) const;

protected:
	static void sdlAudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int);

	SF_INFO audioFileInfo{};
	std::unique_ptr<VAudioFile> audioFileData = nullptr;
	SNDFILE* audioFile = nullptr;
	SDL_AudioStream* audioStream = nullptr;
};

class VAudioPreview final : public VPreviewPlugin_V6_0 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID VPreviewPlugin_V6_0_iid FILE "VAudioPreview.json");
	Q_INTERFACES(VPreviewPlugin_V6_0);

public:
	void initPlugin(VWindowAccess_V6_0*) override;

	[[nodiscard]] const QSet<QString>& getFileExtensions() const override;

	[[nodiscard]] int getPriority() const override;

	[[nodiscard]] QWidget* getPreview(QWidget* parent) const override;

	[[nodiscard]] QString setData(QWidget* preview_, const QString&, const quint8* dataPtr, quint64 dataLen) override;

protected:
	bool sdlAudioInitialized = false;
};
