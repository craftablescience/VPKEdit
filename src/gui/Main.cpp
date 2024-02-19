#include <memory>

#include <QApplication>
#include <QSettings>
#include <QSurfaceFormat>
#include <QTranslator>

#include <vpkedit/Version.h>

#include "config/Options.h"
#include "Window.h"

using namespace vpkedit;

int main(int argc, char** argv) {
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setSamples(4);
	QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

	QTranslator translator;
	if (translator.load(QLocale(), PROJECT_NAME.data(), ".", ":/i18n")) {
		QCoreApplication::installTranslator(&translator);
	}

    QCoreApplication::setOrganizationName(ORGANIZATION_NAME.data());
    QCoreApplication::setApplicationName(PROJECT_NAME.data());
    QCoreApplication::setApplicationVersion(PROJECT_VERSION.data());

#if !defined(__APPLE__) && !defined(_WIN32)
    QGuiApplication::setDesktopFileName(PROJECT_NAME.data());
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
