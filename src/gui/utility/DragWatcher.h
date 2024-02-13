#pragma once

#include <efsw/efsw.hpp>
#include <QObject>

class DragFileUpdateListener : public efsw::FileWatchListener {
	friend class DragWatcher;

public:
	void handleFileAction(efsw::WatchID watchID, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) override;

protected:
	explicit DragFileUpdateListener(QWidget* parent_ = nullptr);

	QWidget* parent;
};

class DragWatcher : public QObject {
	Q_OBJECT;

public:
	explicit DragWatcher(QWidget* parent = nullptr);

private:
	QString watchParentDir;
	QString watchFile;

	efsw::FileWatcher watcher;
	DragFileUpdateListener listener;
};
