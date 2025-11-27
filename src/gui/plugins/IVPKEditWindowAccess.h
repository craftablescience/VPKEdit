#pragma once

#include <QObject>

class QByteArray;
class QSettings;
class QString;

class IVPKEditWindowAccess_V2 : public QObject {
	Q_OBJECT;

public:
	using QObject::QObject;

	[[nodiscard]] virtual QSettings* getOptions() const = 0;

	[[nodiscard]] virtual bool isReadOnly() const = 0;

	[[nodiscard]] virtual bool hasEntry(const QString& entryPath) const = 0;

	[[nodiscard]] virtual bool readBinaryEntry(const QString& entryPath, QByteArray& data) const = 0;

	[[nodiscard]] virtual bool readTextEntry(const QString& entryPath, QString& data) const = 0;

	virtual void selectEntryInEntryTree(const QString& entryPath) const = 0;
};
