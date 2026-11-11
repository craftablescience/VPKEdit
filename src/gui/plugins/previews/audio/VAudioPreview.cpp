#include "VAudioPreview.hpp"

#include <cstring>

#include <SDL3/SDL.h>

SF_VIRTUAL_IO VAudioFile::SF_VIO{
	.get_filelen = [](void* user_data) -> sf_count_t {
		if (const auto* current = static_cast<VAudioFile*>(user_data)) {
			return current->data.size();
		}
		return 0;
	},
	.seek = [](sf_count_t offset, int whence, void* user_data) -> sf_count_t {
		auto* current = static_cast<VAudioFile*>(user_data);
		if (!current) {
			return 0;
		}
		switch (whence) {
			case SEEK_SET:
				return current->head = qBound(static_cast<sf_count_t>(0), offset, static_cast<sf_count_t>(current->data.size()));
			case SEEK_CUR:
				return current->head = qBound(static_cast<sf_count_t>(0), current->head + offset, static_cast<sf_count_t>(current->data.size()));
			case SEEK_END:
				return current->head = qBound(static_cast<sf_count_t>(0), static_cast<sf_count_t>(current->data.size()) - offset, static_cast<sf_count_t>(current->data.size()));
			default:
				break;
		}
		return current->head;
	},
	.read = [](void* ptr, sf_count_t count, void* user_data) -> sf_count_t {
		auto* current = static_cast<VAudioFile*>(user_data);
		if (!current) {
			return 0;
		}
		count = qBound(static_cast<sf_count_t>(0), count, static_cast<sf_count_t>(current->data.size() - current->head - count));
		std::memcpy(ptr, current->data.data() + current->head, count);
		current->head += count;
		return count;
	},
	.write = [](const void* ptr, sf_count_t count, void* user_data) -> sf_count_t {
		auto* current = static_cast<VAudioFile*>(user_data);
		if (!current) {
			return 0;
		}
		if (
			const auto rescale = qBound(static_cast<sf_count_t>(0), count, static_cast<sf_count_t>(current->data.size() - current->head - count));
			rescale > 0
		) {
			current->data.resize(current->data.size() + rescale);
		}
		std::memcpy(current->data.data() + current->head, ptr, count);
		current->head += count;
		return count;
	},
	.tell = [](void* user_data) -> sf_count_t {
		if (const auto* current = static_cast<VAudioFile*>(user_data)) {
			return current->head;
		}
		return 0;
	},
};

VAudioPreviewWidget::VAudioPreviewWidget(QWidget* parent) : QWidget{parent} {}

VAudioPreviewWidget::~VAudioPreviewWidget() {
	if (this->audioFile) {
		sf_close(this->audioFile);
		this->audioFile = nullptr;
	}
}

QString VAudioPreviewWidget::loadAudio(const quint8* dataPtr, quint64 dataLen) {
	if (this->audioStream) {
		SDL_DestroyAudioStream(this->audioStream);
		this->audioStream = nullptr;
	}
	if (this->audioFile) {
		sf_close(this->audioFile);
		this->audioFile = nullptr;
	}

	this->audioFileData = std::make_unique<VAudioFile>(QByteArray{reinterpret_cast<const char*>(dataPtr), static_cast<qint64>(dataLen)}, 0);
	this->audioFileInfo = SF_INFO{};
	this->audioFile = sf_open_virtual(&VAudioFile::SF_VIO, SFM_READ, &this->audioFileInfo, this->audioFileData.get());
	if (!this->audioFile) {
		this->audioFileData.reset();
		return sf_strerror(this->audioFile);
	}

	const SDL_AudioSpec spec{
		.format = SDL_AUDIO_F32,
		.channels = this->audioFileInfo.channels,
		.freq = this->audioFileInfo.samplerate,
	};
	this->audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &sdlAudioCallback, this);
	if (!this->audioStream) {
		return SDL_GetError();
	}
	SDL_ResumeAudioStreamDevice(this->audioStream);

	return {};
}

void VAudioPreviewWidget::seek(sf_count_t frame) const {
	if (!this->audioFile || !this->audioStream) {
		return;
	}
	sf_seek(this->audioFile, qBound(0, frame, this->audioFileInfo.frames), SEEK_SET);
	SDL_FlushAudioStream(this->audioStream);
}

void VAudioPreviewWidget::sdlAudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int) {
	const auto* self = static_cast<VAudioPreviewWidget*>(userdata);
	if (!self) {
		return;
	}
	const auto frames = static_cast<sf_count_t>(additional_amount / (self->audioFileInfo.channels * sizeof(float)));
	if (frames <= 0) {
		return;
	}

	QList<float> frameBuffer(frames * self->audioFileInfo.channels, 0.f);
	if (
		const auto framesRead = sf_readf_float(self->audioFile, frameBuffer.data(), frames);
		framesRead > 0
	) {
		SDL_PutAudioStreamData(stream, frameBuffer.data(), static_cast<int>(framesRead * self->audioFileInfo.channels * sizeof(float)));
	}
}

void VAudioPreview::initPlugin(VWindowAccess_V6_0*) {
	this->sdlAudioInitialized = SDL_Init(SDL_INIT_AUDIO);
}

const QSet<QString>& VAudioPreview::getFileExtensions() const {
	static const QSet<QString> EXTENSIONS{
		"wav",
	};
	return EXTENSIONS;
}

int VAudioPreview::getPriority() const {
	return 50;
}

QWidget* VAudioPreview::getPreview(QWidget* parent) const {
	return new VAudioPreviewWidget{parent};
}

QString VAudioPreview::setData(QWidget* preview_, const QString&, const quint8* dataPtr, quint64 dataLen) {
	if (!this->sdlAudioInitialized) {
		return tr("SDL audio subsystem failed to initialize.");
	}

	auto* preview = qobject_cast<VAudioPreviewWidget*>(preview_);
	if (!preview) {
		return UNEXPECTED_ERROR_MSG.toString();
	}

	if (const auto error = preview->loadAudio(dataPtr, dataLen); !error.isEmpty()) {
		return tr("Error encountered while loading audio:\n\"%1\"").arg(error);
	}
	return {};
}
