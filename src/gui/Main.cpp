#include <memory>

#include <QApplication>
#include <QSettings>
#include <QStyle>

#include "Config.h"
#include "Window.h"
#include "Options.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(VPKEDIT_ORGANIZATION_NAME);
    QCoreApplication::setApplicationName(VPKEDIT_PROJECT_NAME);
    QCoreApplication::setApplicationVersion(VPKEDIT_PROJECT_VERSION);

#if !defined(__APPLE__) && !defined(_WIN32)
    QGuiApplication::setDesktopFileName(VPKEDIT_PROJECT_NAME);
#endif

    std::unique_ptr<QSettings> options;
    if (isStandalone()) {
        options = std::make_unique<QSettings>("config.ini", QSettings::Format::IniFormat);
    } else {
        options = std::make_unique<QSettings>();
    }
    setupOptions(*options);

    auto* window = new Window(*options);
    if (!options->value(OPT_START_MAXIMIZED).toBool()) {
        window->show();
    } else {
        window->showMaximized();
    }

    return QApplication::exec();
}
