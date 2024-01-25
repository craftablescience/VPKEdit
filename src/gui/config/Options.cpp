#include "Options.h"

#include <QApplication>
#include <QFileInfo>
#include <QMetaType>
#include <QStyle>

Q_DECLARE_METATYPE(QStringList)

QSettings* opts = nullptr;

bool Options::isStandalone() {
    QFileInfo nonportable(QApplication::applicationDirPath() + "/.nonportable");
    return !(nonportable.exists() && nonportable.isFile());
}

void Options::setupOptions(QSettings& options) {
    if (!options.contains(OPT_STYLE)) {
        options.setValue(OPT_STYLE, QApplication::style()->name());
    }
    QApplication::setStyle(options.value(OPT_STYLE).toString());

    if (!options.contains(OPT_ENTRY_TREE_AUTO_EXPAND)) {
        options.setValue(OPT_ENTRY_TREE_AUTO_EXPAND, false);
    }

	if (!options.contains(OPT_ENTRY_TREE_AUTO_COLLAPSE)) {
		options.setValue(OPT_ENTRY_TREE_AUTO_COLLAPSE, false);
	}

	if (!options.contains(OPT_ENTRY_TREE_HIDE_ICONS)) {
		options.setValue(OPT_ENTRY_TREE_HIDE_ICONS, false);
	}

    if (!options.contains(OPT_ADVANCED_FILE_PROPS)) {
        options.setValue(OPT_ADVANCED_FILE_PROPS, false);
    }

    if (!options.contains(OPT_START_MAXIMIZED)) {
        options.setValue(OPT_START_MAXIMIZED, false);
    }

	if (!options.contains(STR_OPEN_RECENT)) {
		options.setValue(STR_OPEN_RECENT, QStringList{});
	}

	opts = &options;
}

QSettings* Options::getOptions() {
	return opts;
}

void Options::invert(std::string_view option) {
	set(option, !get<bool>(option));
}
