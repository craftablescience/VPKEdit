#pragma once

#include <string_view>

#include <QSettings>

// Options
constexpr std::string_view OPT_STYLE = "style";
constexpr std::string_view OPT_ENTRY_TREE_AUTO_EXPAND = "entry_list_auto_expand";
constexpr std::string_view OPT_ENTRY_TREE_ALLOW_DIR_DRAG = "entry_list_allow_dir_drag";
constexpr std::string_view OPT_ENTRY_TREE_ALLOW_FILE_DRAG = "entry_list_allow_file_drag";
constexpr std::string_view OPT_ENTRY_TREE_AUTO_COLLAPSE = "entry_list_auto_collapse";
constexpr std::string_view OPT_ENTRY_TREE_HIDE_ICONS = "entry_tree_hide_icons";
constexpr std::string_view OPT_ADVANCED_FILE_PROPS = "adv_mode";
constexpr std::string_view OPT_LANGUAGE_OVERRIDE = "language_override";
constexpr std::string_view OPT_DISABLE_STARTUP_UPDATE_CHECK = "disable_startup_update_check";
constexpr std::string_view OPT_ENABLE_DISCORD_RICH_PRESENCE = "enable_discord_rich_presence";
constexpr std::string_view OPT_DISABLE_STEAM_SCANNER = "disable_steam_scanner";

// Storage
constexpr std::string_view STR_OPEN_RECENT = "open_recent";
constexpr std::string_view STR_IGNORED_UPDATE_VERSION = "ignored_update_version";
constexpr std::string_view STR_VICE_CODE_INDEX = "vice_dialog_code_index";
constexpr std::string_view STR_VICE_CODE_VALUE = "vice_dialog_code_value";

namespace Options {

void setupOptions(QSettings& options);

QSettings* getOptions();

template<typename T>
T get(std::string_view option) {
	return getOptions()->value(option).value<T>();
}

template<typename T>
void set(std::string_view option, T value) {
	getOptions()->setValue(option, value);
}

// Only use for booleans!
void invert(std::string_view option);

} // namespace Options
