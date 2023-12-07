#include <memory>

#include <QApplication>
#include <QSettings>
#include <QSurfaceFormat>

#include "config/Config.h"
#include "config/Options.h"
#include "Window.h"

int main(int argc, char** argv) {
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setSamples(4);
	QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(VPKEDIT_ORGANIZATION_NAME);
    QCoreApplication::setApplicationName(VPKEDIT_PROJECT_NAME);
    QCoreApplication::setApplicationVersion(VPKEDIT_PROJECT_VERSION);

#if !defined(__APPLE__) && !defined(_WIN32)
    QGuiApplication::setDesktopFileName(VPKEDIT_PROJECT_NAME);
#endif

    std::unique_ptr<QSettings> options;
    if (Options::isStandalone()) {
        auto configPath = QApplication::applicationDirPath() + "/config.ini";
        options = std::make_unique<QSettings>(configPath, QSettings::Format::IniFormat);
    } else {
        options = std::make_unique<QSettings>();
    }
    Options::setupOptions(*options);

    auto* window = new Window();
    if (!Options::get<bool>(OPT_START_MAXIMIZED)) {
        window->show();
    } else {
        window->showMaximized();
    }

    return QApplication::exec();
}
