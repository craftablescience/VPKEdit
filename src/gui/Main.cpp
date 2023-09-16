#include <QApplication>
#include <QSettings>
#include <QStyle>

#include "Config.h"
#include "Window.h"
#include "Options.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(VPKTOOL_ORGANIZATION_NAME);
    QCoreApplication::setApplicationName(VPKTOOL_PROJECT_NAME);
    QCoreApplication::setApplicationVersion(VPKTOOL_PROJECT_VERSION);

#if !defined(__APPLE__) && !defined(_WIN32)
    QGuiApplication::setDesktopFileName(VPKTOOL_PROJECT_NAME);
#endif

    QSettings options;
    setupOptions(options);

    auto* window = new Window(options);
    if (!options.value(OPT_START_MAXIMIZED).toBool()) {
        window->show();
    } else {
        window->showMaximized();
    }

    return QApplication::exec();
}
