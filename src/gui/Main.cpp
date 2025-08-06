#include <memory>

#include <QApplication>
#include <QSurfaceFormat>
#include <QTranslator>

#include <Config.h>

#include "utility/Options.h"
#include "Window.h"

int main(int argc, char** argv) {
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setSamples(4);
	QSurfaceFormat::setDefaultFormat(format);

	QApplication app{argc, argv};

	QCoreApplication::setOrganizationName(ORGANIZATION_NAME.data());
	QCoreApplication::setApplicationName(PROJECT_NAME.data());
	QCoreApplication::setApplicationVersion(PROJECT_VERSION.data());

#if !defined(__APPLE__) && !defined(_WIN32)
	QGuiApplication::setDesktopFileName(PROJECT_NAME.data());
#endif

	const auto options = std::make_unique<QSettings>();
	Options::setupOptions(*options);

	const auto languageOverride = Options::get<QString>(OPT_LANGUAGE_OVERRIDE);
	const auto locale = languageOverride.isEmpty() ? QLocale{} : QLocale{languageOverride};
	QTranslator translatorQtBase;
	if (translatorQtBase.load(locale, "qtbase", "_", ":/i18n")) {
		QCoreApplication::installTranslator(&translatorQtBase);
	}
	QTranslator translatorQtHelp;
	if (translatorQtHelp.load(locale, "qt_help", "_", ":/i18n")) {
		QCoreApplication::installTranslator(&translatorQtHelp);
	}
	QTranslator translatorQt;
	if (translatorQt.load(locale, "qt", "_", ":/i18n")) {
		QCoreApplication::installTranslator(&translatorQt);
	}
	QTranslator translator;
	if (translator.load(locale, PROJECT_NAME.data(), "_", ":/i18n")) {
		QCoreApplication::installTranslator(&translator);
	}

	auto* window = new Window;
	options->beginGroup("main_window");
	window->restoreGeometry(options->value("geometry", window->saveGeometry()).toByteArray());
	window->restoreState(options->value("state", window->saveState()).toByteArray());
	if (options->contains("position")) {
		window->move(options->value("position", window->pos()).toPoint());
	}
	window->resize(options->value("size", QSize{900, 500}).toSize());
	if (options->value("maximized", window->isMaximized()).toBool()) {
		window->showMaximized();
	} else {
		window->show();
	}
	options->endGroup();

	return QApplication::exec();
}
