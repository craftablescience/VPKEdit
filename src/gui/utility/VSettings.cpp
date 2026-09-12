#include "VSettings.hpp"

#include <QApplication>
#include <QStyle>

namespace {

QSettings* settings = nullptr;

} // namespace

void VSettings::setup(QSettings& settings_) {
	settings = &settings_;

	if (!settings->contains(BOOL_RAISE_TO_TOP_OPENING_FILE)) {
		settings->setValue(BOOL_RAISE_TO_TOP_OPENING_FILE, true);
	}

	if (!settings->contains(BOOL_ALLOW_MULTIPLE_APP_INSTANCES)) {
		settings->setValue(BOOL_ALLOW_MULTIPLE_APP_INSTANCES, false);
	}

	if (!settings->contains(BOOL_ENABLE_DISCORD_RICH_PRESENCE)) {
		settings->setValue(BOOL_ENABLE_DISCORD_RICH_PRESENCE, true);
	}

	if (!settings->contains(BOOL_ENTRY_TREE_HIDE_ICONS)) {
		settings->setValue(BOOL_ENTRY_TREE_HIDE_ICONS, false);
	}

	if (!settings->contains(STR_STYLE)) {
		settings->setValue(STR_STYLE, QApplication::style()->name());
	} else {
		QApplication::setStyle(settings->value(STR_STYLE).toString());
	}

	if (!settings->contains(STR_LANGUAGE_OVERRIDE)) {
		settings->setValue(STR_LANGUAGE_OVERRIDE, QString{});
	}

	if (!settings->contains(STR_DEFAULT_CREATE_DIALOG_DIR)) {
		settings->setValue(STR_DEFAULT_CREATE_DIALOG_DIR, QString{});
	}

	if (!settings->contains(STR_DEFAULT_OPEN_OR_SAVE_DIALOG_DIR)) {
		settings->setValue(STR_DEFAULT_OPEN_OR_SAVE_DIALOG_DIR, QString{});
	}

	if (!settings->contains(STR_DEFAULT_EXTRACT_DIALOG_DIR)) {
		settings->setValue(STR_DEFAULT_EXTRACT_DIALOG_DIR, QString{});
	}

	if (!settings->contains(INT_DIR_VIEW_MODE)) {
		settings->setValue(INT_DIR_VIEW_MODE, 0);
	}
}

QSettings* VSettings::get() {
	return settings;
}

void VSettings::invert(std::string_view setting) {
	set(setting, !get<bool>(setting));
}
