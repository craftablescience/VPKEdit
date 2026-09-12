#include "VDiscordPresence.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

#include <discord_rpc.h>

namespace {

bool DISCORD_INITIALIZED = false;
bool DISCORD_DIRTY = false;
std::string DISCORD_STATE;
std::string DISCORD_DETAILS;
std::string DISCORD_LARGE_IMAGE;
std::string DISCORD_LARGE_IMAGE_TEXT;
std::string DISCORD_SMALL_IMAGE;
std::string DISCORD_SMALL_IMAGE_TEXT;
std::int64_t DISCORD_START_TIMESTAMP = -1;
std::int64_t DISCORD_END_TIMESTAMP = -1;
VDiscordPresence::ButtonData DISCORD_BUTTON1;
VDiscordPresence::ButtonData DISCORD_BUTTON2;

} // namespace

void VDiscordPresence::init(std::string_view appID) {
	if (DISCORD_INITIALIZED) {
		return;
	}

#ifdef _WIN32
	__try {
#endif
	DiscordEventHandlers handlers{};
	Discord_Initialize(appID.data(), &handlers, 1, nullptr);
	std::atexit(&VDiscordPresence::shutdown);
	DISCORD_INITIALIZED = true;
#ifdef _WIN32
	} __except (1 /*EXCEPTION_EXECUTE_HANDLER*/) {}
#endif
}

void VDiscordPresence::setState(std::string state_) {
	if (state_.length() <= MAXIMUM_STRING_LENGTH) {
		DISCORD_STATE = std::move(state_);
		DISCORD_DIRTY = true;
	}
}

void VDiscordPresence::setDetails(std::string details_) {
	if (details_.length() <= MAXIMUM_STRING_LENGTH) {
		DISCORD_DETAILS = std::move(details_);
		DISCORD_DIRTY = true;
	}
}

void VDiscordPresence::setLargeImage(std::string imageKey) {
	if (imageKey.length() <= MAXIMUM_IMAGE_KEY_LENGTH) {
		DISCORD_LARGE_IMAGE = std::move(imageKey);
		DISCORD_DIRTY = true;
	}
}

void VDiscordPresence::setLargeImageText(std::string text) {
	if (text.length() <= MAXIMUM_STRING_LENGTH) {
		DISCORD_LARGE_IMAGE_TEXT = std::move(text);
		DISCORD_DIRTY = true;
	}
}

void VDiscordPresence::setSmallImage(std::string imageKey) {
	if (imageKey.length() <= MAXIMUM_IMAGE_KEY_LENGTH) {
		DISCORD_SMALL_IMAGE = std::move(imageKey);
		DISCORD_DIRTY = true;
	}
}

void VDiscordPresence::setSmallImageText(std::string text) {
	if (text.length() <= MAXIMUM_STRING_LENGTH) {
		DISCORD_SMALL_IMAGE_TEXT = std::move(text);
		DISCORD_DIRTY = true;
	}
}

void VDiscordPresence::setStartTimestamp(std::int64_t time) {
	DISCORD_START_TIMESTAMP = time;
	DISCORD_DIRTY = true;
}

void VDiscordPresence::setEndTimestamp(std::int64_t time) {
	DISCORD_END_TIMESTAMP = time;
	DISCORD_DIRTY = true;
}

void VDiscordPresence::setTopButton(ButtonData button) {
	DISCORD_BUTTON1 = std::move(button);
	DISCORD_DIRTY = true;
}

void VDiscordPresence::setBottomButton(ButtonData button) {
	DISCORD_BUTTON2 = std::move(button);
	DISCORD_DIRTY = true;
}

void VDiscordPresence::update() {
	if (!DISCORD_INITIALIZED) {
		return;
	}
	if (DISCORD_DIRTY) {
		DiscordRichPresence discordPresence{};

		if (!DISCORD_STATE.empty()) {
			discordPresence.state = DISCORD_STATE.c_str();
		}
		if (!DISCORD_DETAILS.empty()) {
			discordPresence.details = DISCORD_DETAILS.c_str();
		}
		if (DISCORD_START_TIMESTAMP >= 0) {
			discordPresence.startTimestamp = DISCORD_START_TIMESTAMP;
		}
		if (DISCORD_END_TIMESTAMP >= 0) {
			discordPresence.endTimestamp = DISCORD_END_TIMESTAMP;
		}
		if (!DISCORD_LARGE_IMAGE.empty()) {
			discordPresence.largeImageKey = DISCORD_LARGE_IMAGE.c_str();
		}
		if (!DISCORD_LARGE_IMAGE_TEXT.empty()) {
			discordPresence.largeImageText = DISCORD_LARGE_IMAGE_TEXT.c_str();
		}
		if (!DISCORD_SMALL_IMAGE.empty()) {
			discordPresence.smallImageKey = DISCORD_SMALL_IMAGE.c_str();
		}
		if (!DISCORD_SMALL_IMAGE_TEXT.empty()) {
			discordPresence.smallImageText = DISCORD_SMALL_IMAGE_TEXT.c_str();
		}

		DiscordButton buttons[2] {{}, {}};
		if (!DISCORD_BUTTON1.url.empty()) {
			buttons[0].label = DISCORD_BUTTON1.name.c_str();
			buttons[0].url = DISCORD_BUTTON1.url.c_str();
		}
		if (!DISCORD_BUTTON2.url.empty()) {
			buttons[1].label = DISCORD_BUTTON2.name.c_str();
			buttons[1].url = DISCORD_BUTTON2.url.c_str();
		}
		discordPresence.buttons = buttons;

		discordPresence.instance = 0;
		Discord_UpdatePresence(&discordPresence);
		DISCORD_DIRTY = false;
	}
#ifdef DISCORD_DISABLE_IO_THREAD
	Discord_UpdateConnection();
#endif
	Discord_RunCallbacks();
}

void VDiscordPresence::reset() {
	DISCORD_STATE = "";
	DISCORD_DETAILS = "";
	DISCORD_LARGE_IMAGE = "";
	DISCORD_LARGE_IMAGE_TEXT = "";
	DISCORD_SMALL_IMAGE = "";
	DISCORD_SMALL_IMAGE_TEXT = "";
	DISCORD_START_TIMESTAMP = -1;
	DISCORD_END_TIMESTAMP = -1;
	DISCORD_BUTTON1.name = "";
	DISCORD_BUTTON1.url = "";
	DISCORD_BUTTON2.name = "";
	DISCORD_BUTTON2.url = "";

	Discord_ClearPresence();
	DISCORD_DIRTY = false;
}

void VDiscordPresence::shutdown() {
	if (DISCORD_INITIALIZED) {
		Discord_Shutdown();
		DISCORD_INITIALIZED = false;
	}
}
