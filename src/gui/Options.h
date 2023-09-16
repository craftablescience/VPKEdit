#pragma once

class QSettings;

constexpr auto OPT_STYLE = "style";
constexpr auto OPT_ENTRY_LIST_AUTO_EXPAND = "entry_list_auto_expand";
constexpr auto OPT_ADV_MODE = "adv_mode";
constexpr auto OPT_START_MAXIMIZED = "start_maximized";

void setupOptions(QSettings& options);
