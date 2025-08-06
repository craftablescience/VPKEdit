#include "DiscordPresence.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

#include <discord_rpc.h>

bool g_DiscordInitialized = false;
bool g_DiscordDirty = false;
std::string g_DiscordState;
std::string g_DiscordDetails;
std::string g_DiscordLargeImage;
std::string g_DiscordLargeImageText;
std::string g_DiscordSmallImage;
std::string g_DiscordSmallImageText;
std::int64_t g_DiscordStartTimestamp = -1;
std::int64_t g_DiscordEndTimestamp = -1;
DiscordPresence::ButtonData g_DiscordButton1;
DiscordPresence::ButtonData g_DiscordButton2;

void DiscordPresence::init(std::string_view appID) {
	if (g_DiscordInitialized) {
		return;
	}

#ifdef _WIN32
	__try {
#endif
	DiscordEventHandlers handlers{};
	Discord_Initialize(appID.data(), &handlers, 1, nullptr);
	std::atexit(&DiscordPresence::shutdown);
	g_DiscordInitialized = true;
#ifdef _WIN32
	} __except (1 /*EXCEPTION_EXECUTE_HANDLER*/) {}
#endif
}

void DiscordPresence::setState(std::string state_) {
	if (state_.length() <= MAXIMUM_STRING_LENGTH) {
		g_DiscordState = std::move(state_);
		g_DiscordDirty = true;
	}
}

void DiscordPresence::setDetails(std::string details_) {
	if (details_.length() <= MAXIMUM_STRING_LENGTH) {
		g_DiscordDetails = std::move(details_);
		g_DiscordDirty = true;
	}
}

void DiscordPresence::setLargeImage(std::string imageKey) {
	if (imageKey.length() <= MAXIMUM_IMAGE_KEY_LENGTH) {
		g_DiscordLargeImage = std::move(imageKey);
		g_DiscordDirty = true;
	}
}

void DiscordPresence::setLargeImageText(std::string text) {
	if (text.length() <= MAXIMUM_STRING_LENGTH) {
		g_DiscordLargeImageText = std::move(text);
		g_DiscordDirty = true;
	}
}

void DiscordPresence::setSmallImage(std::string imageKey) {
	if (imageKey.length() <= MAXIMUM_IMAGE_KEY_LENGTH) {
		g_DiscordSmallImage = std::move(imageKey);
		g_DiscordDirty = true;
	}
}

void DiscordPresence::setSmallImageText(std::string text) {
	if (text.length() <= MAXIMUM_STRING_LENGTH) {
		g_DiscordSmallImageText = std::move(text);
		g_DiscordDirty = true;
	}
}

void DiscordPresence::setStartTimestamp(std::int64_t time) {
	g_DiscordStartTimestamp = time;
	g_DiscordDirty = true;
}

void DiscordPresence::setEndTimestamp(std::int64_t time) {
	g_DiscordEndTimestamp = time;
	g_DiscordDirty = true;
}

void DiscordPresence::setTopButton(ButtonData button) {
	g_DiscordButton1 = std::move(button);
	g_DiscordDirty = true;
}

void DiscordPresence::setBottomButton(ButtonData button) {
	g_DiscordButton2 = std::move(button);
	g_DiscordDirty = true;
}

void DiscordPresence::update() {
	if (!g_DiscordInitialized) {
		return;
	}
	if (g_DiscordDirty) {
		DiscordRichPresence discordPresence{};

		if (!g_DiscordState.empty()) {
			discordPresence.state = g_DiscordState.c_str();
		}
		if (!g_DiscordDetails.empty()) {
			discordPresence.details = g_DiscordDetails.c_str();
		}
		if (g_DiscordStartTimestamp >= 0) {
			discordPresence.startTimestamp = g_DiscordStartTimestamp;
		}
		if (g_DiscordEndTimestamp >= 0) {
			discordPresence.endTimestamp = g_DiscordEndTimestamp;
		}
		if (!g_DiscordLargeImage.empty()) {
			discordPresence.largeImageKey = g_DiscordLargeImage.c_str();
		}
		if (!g_DiscordLargeImageText.empty()) {
			discordPresence.largeImageText = g_DiscordLargeImageText.c_str();
		}
		if (!g_DiscordSmallImage.empty()) {
			discordPresence.smallImageKey = g_DiscordSmallImage.c_str();
		}
		if (!g_DiscordSmallImageText.empty()) {
			discordPresence.smallImageText = g_DiscordSmallImageText.c_str();
		}

		DiscordButton buttons[2] {{}, {}};
		if (!g_DiscordButton1.url.empty()) {
			buttons[0].label = g_DiscordButton1.name.c_str();
			buttons[0].url = g_DiscordButton1.url.c_str();
		}
		if (!g_DiscordButton2.url.empty()) {
			buttons[1].label = g_DiscordButton2.name.c_str();
			buttons[1].url = g_DiscordButton2.url.c_str();
		}
		discordPresence.buttons = buttons;

		discordPresence.instance = 0;
		Discord_UpdatePresence(&discordPresence);
		g_DiscordDirty = false;
	}
#ifdef DISCORD_DISABLE_IO_THREAD
	Discord_UpdateConnection();
#endif
	Discord_RunCallbacks();
}

void DiscordPresence::reset() {
	g_DiscordState = "";
	g_DiscordDetails = "";
	g_DiscordLargeImage = "";
	g_DiscordLargeImageText = "";
	g_DiscordSmallImage = "";
	g_DiscordSmallImageText = "";
	g_DiscordStartTimestamp = -1;
	g_DiscordEndTimestamp = -1;
	g_DiscordButton1.name = "";
	g_DiscordButton1.url = "";
	g_DiscordButton2.name = "";
	g_DiscordButton2.url = "";

	Discord_ClearPresence();
	g_DiscordDirty = false;
}

void DiscordPresence::shutdown() {
	if (g_DiscordInitialized) {
		Discord_Shutdown();
		g_DiscordInitialized = false;
	}
}
