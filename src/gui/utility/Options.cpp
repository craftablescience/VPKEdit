#include "Options.h"

#include <QApplication>
#include <QFileInfo>
#include <QMetaType>
#include <QStyle>

Q_DECLARE_METATYPE(QStringList)

QSettings* opts = nullptr;

void Options::setupOptions(QSettings& options) {
    #define VPKEDIT_OPTION_DEFAULT(key, defaultValue); \
        do { \
            if (!options.contains(key)) { \
                options.setValue(key, defaultValue); \
            } \
        } while (false)

    VPKEDIT_OPTION_DEFAULT(OPT_STYLE, QApplication::style()->name());
	QApplication::setStyle(options.value(OPT_STYLE).toString());

	VPKEDIT_OPTION_DEFAULT(OPT_ENTRY_TREE_AUTO_EXPAND, false);
	VPKEDIT_OPTION_DEFAULT(OPT_ENTRY_TREE_ALLOW_DIR_DRAG, true);
	VPKEDIT_OPTION_DEFAULT(OPT_ENTRY_TREE_ALLOW_FILE_DRAG, true);
	VPKEDIT_OPTION_DEFAULT(OPT_ENTRY_TREE_AUTO_COLLAPSE, false);
	VPKEDIT_OPTION_DEFAULT(OPT_ENTRY_TREE_HIDE_ICONS, false);
	VPKEDIT_OPTION_DEFAULT(OPT_ADVANCED_FILE_PROPS, false);
	VPKEDIT_OPTION_DEFAULT(OPT_LANGUAGE_OVERRIDE, QString{});
	VPKEDIT_OPTION_DEFAULT(OPT_DISABLE_STARTUP_UPDATE_CHECK, false)
	VPKEDIT_OPTION_DEFAULT(OPT_ENABLE_DISCORD_RICH_PRESENCE, true);
	VPKEDIT_OPTION_DEFAULT(OPT_DISABLE_STEAM_SCANNER, true);
	VPKEDIT_OPTION_DEFAULT(STR_OPEN_RECENT, QStringList{});
	VPKEDIT_OPTION_DEFAULT(STR_IGNORED_UPDATE_VERSION, QString{});

	opts = &options;
}

QSettings* Options::getOptions() {
	return opts;
}

void Options::invert(std::string_view option) {
	set(option, !get<bool>(option));
}
