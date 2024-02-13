#pragma once

#include <cstddef>
#include <string_view>

namespace AudioPlayer {

bool initialized();

std::string_view initAudio(const void* data, std::size_t len);

void pause();

void play();

void setVolume(float volume);

std::size_t getPositionInFrames();

double getPositionInSeconds();

std::size_t getLengthInFrames();

double getLengthInSeconds();

std::uint32_t getSampleRate();

std::uint32_t getChannelCount();

void seekToFrame(std::int64_t frame);

void deinitAudio();

} // namespace AudioPlayer
