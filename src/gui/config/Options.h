#pragma once

#include <string_view>

#include <QSettings>

constexpr std::string_view OPT_STYLE = "style";
constexpr std::string_view OPT_ENTRY_LIST_AUTO_EXPAND = "entry_list_auto_expand";
constexpr std::string_view OPT_ADVANCED_FILE_PROPS = "adv_mode";
constexpr std::string_view OPT_START_MAXIMIZED = "start_maximized";

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
