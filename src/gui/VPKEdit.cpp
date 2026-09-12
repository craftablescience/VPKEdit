#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSurfaceFormat>
#include <QTranslator>

#include "Config.hpp"

#include "utility/VSettings.hpp"
#include "windows/VMainWindow.hpp"

int main(int argc, char* argv[]) {
	QSurfaceFormat format;
#ifdef DEBUG
	format.setOption(QSurfaceFormat::DebugContext);
#endif
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setDepthBufferSize(24);
	format.setSamples(4);
	QSurfaceFormat::setDefaultFormat(format);

	QApplication app{argc, argv};

	QCoreApplication::setOrganizationName(PROJECT_ORGANIZATION_NAME);
	QCoreApplication::setApplicationName(PROJECT_NAME);
	QCoreApplication::setApplicationVersion(PROJECT_VERSION);

#if !defined(Q_OS_APPLE) && !defined(Q_OS_WIN)
	QGuiApplication::setDesktopFileName(PROJECT_NAME);
#endif

	QSettings settings;
	VSettings::setup(settings);

	QLocalServer server;
	if (!VSettings::get<bool>(VSettings::BOOL_ALLOW_MULTIPLE_APP_INSTANCES)) {
		const auto serverName = QString{"%1_%2_%3"}.arg(PROJECT_ORGANIZATION_NAME, PROJECT_NAME, PROJECT_VERSION);
		QLocalSocket socket;
		socket.connectToServer(serverName);
		if (socket.waitForConnected(500)) {
			QByteArray data;
			QDataStream stream{&data, QIODevice::WriteOnly};
			stream << QCoreApplication::arguments();
			socket.write(data);
			socket.waitForBytesWritten(500);
			return 0;
		}

		QLocalServer::removeServer(serverName);
		QObject::connect(&server, &QLocalServer::newConnection, [&server] {
			auto* client = server.nextPendingConnection();
			QObject::connect(client, &QLocalSocket::readyRead, [client] {
				if (g_MainWindow) {
					const QByteArray data = client->readAll();
					QDataStream stream{data};
					QStringList args;
					stream >> args;
					for (int i = 1; i < args.size(); i++) {
						g_MainWindow->addPackFile(args[i]);
					}

					if (VSettings::get<bool>(VSettings::BOOL_RAISE_TO_TOP_OPENING_FILE))
#ifdef Q_OS_WIN
					{
						const auto flags = g_ManeWindow->windowFlags();
						g_ManeWindow->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
						g_ManeWindow->show();
						g_ManeWindow->setWindowFlags(flags);
					}
					// Will simply flash in taskbar if above code didn't run
#endif
					{
						// Yes we need all of these
						g_MainWindow->show();
						g_MainWindow->raise();
						g_MainWindow->activateWindow();
					}
				}
				client->deleteLater();
			});
		});
		server.listen(serverName);
	}

	const auto languageOverride = VSettings::get<QString>(VSettings::STR_LANGUAGE_OVERRIDE);
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
	if (translator.load(locale, PROJECT_NAME, "_", ":/i18n")) {
		QCoreApplication::installTranslator(&translator);
	}

	// Show a window
	g_MainWindow = new VMainWindow;
	for (int i = 1; i < argc; i++) {
		g_MainWindow->addPackFile(argv[i]);
	}
	g_MainWindow->show();

	return QApplication::exec();
}
