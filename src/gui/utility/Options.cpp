#include "Options.h"

#include <QApplication>
#include <QFileInfo>
#include <QMetaType>
#include <QStyle>
#include <vcryptpp/vcryptpp.h>

using namespace vcryptpp;

Q_DECLARE_METATYPE(QStringList)

QSettings* opts = nullptr;

bool Options::isStandalone() {
#ifdef VPKEDIT_BUILD_FOR_STRATA_SOURCE
	// Standalone mode is only used to check if we should write a physical config file.
	// If we're building for a Strata Source game, we should just use the system registry.
	// No need to pollute the bin folder!
	return false;
#else
	QFileInfo nonportable(QApplication::applicationDirPath() + "/.nonportable");
	return !(nonportable.exists() && nonportable.isFile());
#endif
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

	if (!options.contains(OPT_LANGUAGE_OVERRIDE)) {
		options.setValue(OPT_LANGUAGE_OVERRIDE, QString());
	}

	if (!options.contains(OPT_DISABLE_STARTUP_UPDATE_CHECK)) {
		options.setValue(OPT_DISABLE_STARTUP_UPDATE_CHECK, false);
	}

	if (!options.contains(OPT_ENABLE_DISCORD_RICH_PRESENCE)) {
		options.setValue(OPT_ENABLE_DISCORD_RICH_PRESENCE, true);
	}

	if (!options.contains(OPT_DISABLE_STEAM_SCANNER)) {
		options.setValue(OPT_DISABLE_STEAM_SCANNER, false);
	}

	if (!options.contains(STR_OPEN_RECENT)) {
		options.setValue(STR_OPEN_RECENT, QStringList{});
	}

	if (!options.contains(STR_IGNORED_UPDATE_VERSION)) {
		options.setValue(STR_IGNORED_UPDATE_VERSION, QString());
	}

	if (!options.contains(STR_VICE_CODE_INDEX)) {
		options.setValue(STR_VICE_CODE_INDEX, 0);
	}

	if (!options.contains(STR_VICE_CODE_VALUE)) {
		options.setValue(STR_VICE_CODE_VALUE, QString(VICE::KnownCodes::DEFAULT.data()));
	}

	opts = &options;
}

QSettings* Options::getOptions() {
	return opts;
}

void Options::invert(std::string_view option) {
	set(option, !get<bool>(option));
}
