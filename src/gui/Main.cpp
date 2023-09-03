#include <QApplication>
#include <QSettings>
#include <QStyle>

#include "Config.h"
#include "Window.h"

static inline void setUpOptions(QSettings& options) {
    if (!options.contains(OPT_STYLE)) {
        options.setValue(OPT_STYLE, QApplication::style()->name());
    }
    QApplication::setStyle(options.value(OPT_STYLE).toString());

    if (!options.contains(OPT_START_MAXIMIZED)) {
        options.setValue(OPT_START_MAXIMIZED, false);
    }
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QSettings options("craftablescience", VPKTOOL_PROJECT_NAME);
    setUpOptions(options);

    auto* window = new Window(options);
    if (!options.value(OPT_START_MAXIMIZED).toBool()) {
        window->show();
    } else {
        window->showMaximized();
    }

    return QApplication::exec();
}
