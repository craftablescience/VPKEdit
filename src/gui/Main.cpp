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

    if (!options.contains(OPT_ENTRY_LIST_AUTO_EXPAND)) {
        options.setValue(OPT_ENTRY_LIST_AUTO_EXPAND, false);
    }

    if (!options.contains(OPT_START_MAXIMIZED)) {
        options.setValue(OPT_START_MAXIMIZED, false);
    }
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(VPKTOOL_ORGANIZATION_NAME);
    QCoreApplication::setApplicationName(VPKTOOL_PROJECT_NAME);
    QCoreApplication::setApplicationVersion(VPKTOOL_PROJECT_VERSION);

#if !defined(__APPLE__) && !defined(_WIN32)
    QGuiApplication::setDesktopFileName(VPKTOOL_PROJECT_NAME);
#endif

    QSettings options;
    setUpOptions(options);

    auto* window = new Window(options);
    if (!options.value(OPT_START_MAXIMIZED).toBool()) {
        window->show();
    } else {
        window->showMaximized();
    }

    return QApplication::exec();
}
