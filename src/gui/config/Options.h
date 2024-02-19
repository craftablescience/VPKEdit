#pragma once

#include <string_view>

#include <QSettings>

// Options
constexpr std::string_view OPT_STYLE = "style";
constexpr std::string_view OPT_ENTRY_TREE_AUTO_EXPAND = "entry_list_auto_expand";
constexpr std::string_view OPT_ENTRY_TREE_AUTO_COLLAPSE = "entry_list_auto_collapse";
constexpr std::string_view OPT_ENTRY_TREE_HIDE_ICONS = "entry_tree_hide_icons";
constexpr std::string_view OPT_ADVANCED_FILE_PROPS = "adv_mode";
constexpr std::string_view OPT_START_MAXIMIZED = "start_maximized";
constexpr std::string_view OPT_FORCE_ENGLISH = "force_english";

// Storage
constexpr std::string_view STR_OPEN_RECENT = "open_recent";

namespace Options {

bool isStandalone();

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
