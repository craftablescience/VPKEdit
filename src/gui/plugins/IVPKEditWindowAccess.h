#pragma once

#include <QObject>

class IVPKEditWindowAccess_V1 : public QObject {
	Q_OBJECT;

public:
	using QObject::QObject;

	[[nodiscard]] virtual bool hasEntry(const QString& entryPath) = 0;

	[[nodiscard]] virtual bool readBinaryEntry(const QString& entryPath, QByteArray& data) = 0;

	[[nodiscard]] virtual bool readTextEntry(const QString& entryPath, QString& data) = 0;

	virtual void selectEntryInEntryTree(const QString& entryPath) = 0;
};
