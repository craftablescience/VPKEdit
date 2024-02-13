#include "DragWatcher.h"

#include <filesystem>
#include <string_view>

#include <QDir>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QRandomGenerator>

#include <vpkedit/detail/FileStream.h>

using namespace vpkedit::detail;

namespace {

QString getRandomString() {
	static constexpr std::string_view POSSIBLE_CHARS = "abcdefghijklmnopqrstuvwxyz_";
	static constexpr int LENGTH = 8;

	QString out = "d";
	for(int i = 0; i < LENGTH - 2; i++) {
		auto index = QRandomGenerator::global()->generate() % POSSIBLE_CHARS.length();
		out += POSSIBLE_CHARS.at(index);
	}
	return out + "d";
}

} // namespace

DragFileUpdateListener::DragFileUpdateListener(QWidget* parent_)
		: parent(parent_) {}

void DragFileUpdateListener::handleFileAction(efsw::WatchID watchID, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) {
	QMessageBox::critical(this->parent, (dir + '/' + filename).c_str(), oldFilename.c_str());
}

DragWatcher::DragWatcher(QWidget* parent)
		: QObject(parent)
		, watchParentDir((std::filesystem::temp_directory_path() / "vpkedit_drag").string().c_str())
		, watchFile("vpkedit_drag_target." + ::getRandomString())
		, watcher()
		, listener(parent) {
	const auto failure = [parent] {
		QMessageBox::critical(parent, tr("Error"), tr("Failed to create drag target: please tell a programmer!"));
	};
	if (QDir dir; !dir.mkpath(this->watchParentDir)) {
		failure();
		return;
	}
	QString fullPath = this->watchParentDir + QDir::separator() + this->watchFile;
	{
		FileStream out{fullPath.toStdString(), FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
	}
	if (this->watcher.addWatch(this->watchParentDir.toStdString(), &this->listener, false) < 0) {
		failure();
		return;
	}
	this->watcher.watch();
}
