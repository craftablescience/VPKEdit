#pragma once

#include <string_view>

#include <QSettings>

namespace VSettings {

#define VPKEDIT_SETTING(name) constexpr const char* name = #name

// Settings
VPKEDIT_SETTING(BOOL_RAISE_TO_TOP_OPENING_FILE);
VPKEDIT_SETTING(BOOL_ALLOW_MULTIPLE_APP_INSTANCES);
VPKEDIT_SETTING(BOOL_ENABLE_DISCORD_RICH_PRESENCE);
VPKEDIT_SETTING(BOOL_ENTRY_TREE_HIDE_ICONS);
VPKEDIT_SETTING(STR_STYLE);
VPKEDIT_SETTING(STR_LANGUAGE_OVERRIDE);

// Storage
VPKEDIT_SETTING(STR_DEFAULT_CREATE_DIALOG_DIR);
VPKEDIT_SETTING(STR_DEFAULT_OPEN_OR_SAVE_DIALOG_DIR);
VPKEDIT_SETTING(STR_DEFAULT_EXTRACT_DIALOG_DIR);
VPKEDIT_SETTING(INT_DIR_VIEW_MODE);

#undef VPKEDIT_SETTING

void setup(QSettings& settings_);

QSettings* get();

template<typename T>
T get(std::string_view setting) {
	return get()->value(setting).value<T>();
}

template<typename T>
void set(std::string_view setting, T value) {
	get()->setValue(setting, value);
}

// Only use for booleans!
void invert(std::string_view setting);

} // namespace VSettings
