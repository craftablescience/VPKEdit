#pragma once

#include <QObject>

class QByteArray;
class QSettings;
class QString;

class IVPKEditWindowAccess_V3 : public QObject {
	Q_OBJECT;

public:
	using QObject::QObject;

	[[nodiscard]] virtual QSettings* getOptions() const = 0;

	[[nodiscard]] virtual bool isReadOnly() const = 0;

	virtual void addFile(bool showOptions, const QString& startDir = QString(), const QString& filePath = QString()) const = 0;

	virtual void addDir(bool showOptions, const QString& startDir = QString(), const QString& dirPath = QString()) const = 0;

	virtual bool removeFile(const QString& path) const = 0;

	virtual void removeDir(const QString& path) const = 0;

	virtual void editFileContents(const QString& path, const QByteArray& data) const = 0;

	virtual void editFileContents(const QString& path, const QString& data) const = 0;

	virtual void renameDir(const QString& oldPath, const QString& newPath = QString()) const = 0;

	[[nodiscard]] virtual bool readBinaryEntry(const QString& entryPath, QByteArray& data) const = 0;

	[[nodiscard]] virtual bool readTextEntry(const QString& entryPath, QString& data) const = 0;

	virtual void selectEntryInEntryTree(const QString& entryPath) const = 0;

	[[nodiscard]] virtual bool hasEntry(const QString& entryPath) const = 0;

	virtual void selectSubItemInDir(const QString& path) const = 0;

	virtual void extractFile(const QString& path, QString savePath = QString()) const = 0;

	virtual void extractDir(const QString& path, const QString& saveDir = QString()) const = 0;

	virtual void extractPaths(const QStringList& paths, const QString& saveDir = QString()) const = 0;

	virtual void extractAll(QString saveDir = QString()) const = 0;
};
