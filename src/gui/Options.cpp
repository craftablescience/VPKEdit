#include "Options.h"

#include <QApplication>
#include <QFileInfo>
#include <QSettings>
#include <QStyle>

bool isStandalone() {
    QFileInfo nonportable(QApplication::applicationDirPath() + "/.nonportable");
    return !(nonportable.exists() && nonportable.isFile());
}

void setupOptions(QSettings& options) {
    if (!options.contains(OPT_STYLE)) {
        options.setValue(OPT_STYLE, QApplication::style()->name());
    }
    QApplication::setStyle(options.value(OPT_STYLE).toString());

    if (!options.contains(OPT_ENTRY_LIST_AUTO_EXPAND)) {
        options.setValue(OPT_ENTRY_LIST_AUTO_EXPAND, false);
    }

    if (!options.contains(OPT_ADV_MODE)) {
        options.setValue(OPT_ADV_MODE, false);
    }

    if (!options.contains(OPT_START_MAXIMIZED)) {
        options.setValue(OPT_START_MAXIMIZED, false);
    }
}
