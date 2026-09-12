#pragma once

#include "previews/VPreviewDirectoryParent.hpp"
#include "VEntryTreeView.hpp"

class QHBoxLayout;
class QLineEdit;
class QStackedLayout;

class VNavigationBar : public QWidget {
	Q_OBJECT;

public:
	explicit VNavigationBar(QWidget* parent = nullptr);

	void setPath(const QString& path);

	void clearPreview();

signals:
	void pathModified(const QString& path);

	void dirViewModeUpdated(VPreviewDirectoryParent::ViewMode mode);

public slots:
	void onDirSelected(const QString& path, vpkpp::Attribute attrs, const QStringList&, const QMap<QString, std::optional<vpkpp::Entry>>&);

	void onFileSelected(const QString& path, const QByteArray&);

protected:
	QStackedLayout* navigationBarLayout;
	QLineEdit* pathEdit;
	QList<QWidget*> components;
	QHBoxLayout* componentLayout;
};
