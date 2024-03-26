#pragma once

#include <string>
#include <string_view>

namespace DiscordPresence {

constexpr int MAXIMUM_STRING_LENGTH = 127;
constexpr int MAXIMUM_IMAGE_KEY_LENGTH = 31;

struct ButtonData {
	std::string name;
	std::string url;
};

void init(std::string_view appID);

void setState(std::string state_);

void setDetails(std::string details_);

void setLargeImage(std::string imageKey);

void setLargeImageText(std::string text);

void setSmallImage(std::string imageKey);

void setSmallImageText(std::string text);

void setStartTimestamp(std::int64_t time);

void setEndTimestamp(std::int64_t time);

void setTopButton(ButtonData button);

void setBottomButton(ButtonData button);

void update();

void reset();

void shutdown();

} // namespace DiscordPresence
