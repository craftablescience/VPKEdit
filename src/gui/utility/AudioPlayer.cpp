#include "AudioPlayer.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

// This is ugly I hate it
ma_decoder g_MiniAudioDecoder;
ma_device g_MiniAudioDevice;
bool g_MiniAudioIsInitialized = false;
std::size_t g_MiniAudioCurrentPosition = 0;
std::int64_t g_MiniAudioSeekPosition = -1;

namespace {

void dataCallback(ma_device* device, void* output, const void* /*pInput*/, ma_uint32 frameCount) {
	auto* decoder = static_cast<ma_decoder*>(device->pUserData);
	if (!decoder) {
		return;
	}

	ma_uint64 numFrames = 0;
	ma_decoder_read_pcm_frames(decoder, output, frameCount, &numFrames);
	g_MiniAudioCurrentPosition += frameCount;
	if (numFrames == 0) {
		// Make sure
		g_MiniAudioCurrentPosition = AudioPlayer::getLengthInFrames();
	}

	if (g_MiniAudioSeekPosition >= 0) {
		ma_decoder_seek_to_pcm_frame(decoder, g_MiniAudioSeekPosition);
		g_MiniAudioCurrentPosition = g_MiniAudioSeekPosition;
		g_MiniAudioSeekPosition = -1;
	}
}

} // namespace

bool AudioPlayer::initialized() {
	return g_MiniAudioIsInitialized;
}

QString AudioPlayer::initAudio(const void* data, std::size_t len) {
	if (g_MiniAudioIsInitialized) {
		deinitAudio();
	}

	ma_device_config deviceConfig;

	if (ma_decoder_init_memory(data, len, nullptr, &g_MiniAudioDecoder) != MA_SUCCESS) {
		return QObject::tr("Failed to initialize decoder.");
	}

	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format   = g_MiniAudioDecoder.outputFormat;
	deviceConfig.playback.channels = g_MiniAudioDecoder.outputChannels;
	deviceConfig.sampleRate        = g_MiniAudioDecoder.outputSampleRate;
	deviceConfig.dataCallback      = dataCallback;
	deviceConfig.pUserData         = &g_MiniAudioDecoder;

	if (ma_device_init(nullptr, &deviceConfig, &g_MiniAudioDevice) != MA_SUCCESS) {
		ma_decoder_uninit(&g_MiniAudioDecoder);
		return QObject::tr("Failed to open playback device.");
	}

	g_MiniAudioIsInitialized = true;
	g_MiniAudioCurrentPosition = 0;
	g_MiniAudioSeekPosition = -1;

	play();

	return "";
}

void AudioPlayer::pause() {
	if (!g_MiniAudioIsInitialized)
		return;
	if (ma_device_stop(&g_MiniAudioDevice) != MA_SUCCESS) {
		deinitAudio();
	}
}

void AudioPlayer::play() {
	if (!g_MiniAudioIsInitialized)
		return;
	if (ma_device_start(&g_MiniAudioDevice) != MA_SUCCESS) {
		deinitAudio();
	}
}

void AudioPlayer::setVolume(float volume) {
	if (!g_MiniAudioIsInitialized)
		return;
	if (ma_device_set_master_volume(&g_MiniAudioDevice, volume) != MA_SUCCESS) {
		deinitAudio();
	}
}


std::size_t AudioPlayer::getPositionInFrames() {
	if (!g_MiniAudioIsInitialized)
		return 0;
	return g_MiniAudioCurrentPosition;
}

double AudioPlayer::getPositionInSeconds() {
	return static_cast<double>(getPositionInFrames()) / static_cast<double>(getSampleRate());
}

std::size_t AudioPlayer::getLengthInFrames() {
	if (!g_MiniAudioIsInitialized)
		return 0;
	ma_uint64 pcmFrameCount;
	if (ma_decoder_get_length_in_pcm_frames(&g_MiniAudioDecoder, &pcmFrameCount) != MA_SUCCESS) {
		deinitAudio();
		return 0;
	}
	return pcmFrameCount;
}

double AudioPlayer::getLengthInSeconds() {
	return static_cast<double>(getLengthInFrames()) / static_cast<double>(getSampleRate());
}

std::uint32_t AudioPlayer::getSampleRate() {
	if (!g_MiniAudioIsInitialized)
		return 0;
	return g_MiniAudioDevice.sampleRate;
}

std::uint32_t AudioPlayer::getChannelCount() {
	if (!g_MiniAudioIsInitialized)
		return 0;
	return g_MiniAudioDecoder.outputChannels;
}

void AudioPlayer::seekToFrame(std::int64_t frame) {
	if (!g_MiniAudioIsInitialized)
		return;
	g_MiniAudioSeekPosition = frame;
	// Hack
	g_MiniAudioCurrentPosition = g_MiniAudioSeekPosition;
}

void AudioPlayer::deinitAudio() {
	if (!g_MiniAudioIsInitialized)
		return;
	ma_device_uninit(&g_MiniAudioDevice);
	ma_decoder_uninit(&g_MiniAudioDecoder);
	g_MiniAudioIsInitialized = false;
	g_MiniAudioCurrentPosition = 0;
	g_MiniAudioSeekPosition = -1;
}
