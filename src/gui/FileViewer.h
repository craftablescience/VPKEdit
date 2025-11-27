#pragma once

#include <QPluginLoader>
#include <QWidget>

#include "plugins/previews/IVPKEditPreviewPlugin.h"

class QAction;
class QLineEdit;
class QToolButton;

class DirPreview;
class EmptyPreview;
class InfoPreview;
class TextPreview;
class TexturePreview;

namespace vpkpp {

class Entry;
class PackFile;

} // namespace vpkpp

class Window;

class NavBar : public QWidget {
	Q_OBJECT;

public:
	// This should be more than enough for anybody
	static constexpr int NAVIGATION_HISTORY_LIMIT = 512;

	explicit NavBar(Window* window_, QWidget* parent = nullptr);

	[[nodiscard]] QString path() const;

	void setPath(const QString& newPath);

	void navigateBack();

	void navigateNext();

	void navigateUp();

	void navigateHome();

	void navigatePath();

	void clearContents(bool resetHistory = true);

signals:
	void pathChanged(const QString& newPath);

private:
	Window* window;

	QToolButton* backButton;
	QToolButton* nextButton;
	QToolButton* upButton;
	QToolButton* homeButton;
	QAction* backButtonAction;
	QAction* nextButtonAction;
	QAction* upButtonAction;
	QAction* homeButtonAction;
	QLineEdit* currentPath;

	QList<QString> history;
	int historyIndex;

	void resetButtonIcons() const;

	void processPathChanged(const QString& newPath, bool addToHistory = true, bool firePathChanged = true);
};

class FileViewer;

class VPKEditWindowAccess_V2 final : public IVPKEditWindowAccess_V2 {
public:
	explicit VPKEditWindowAccess_V2(FileViewer* fileViewer_);

	[[nodiscard]] QSettings* getOptions() const override;

	[[nodiscard]] bool isReadOnly() const override;

	[[nodiscard]] bool hasEntry(const QString& entryPath) const override;

	[[nodiscard]] bool readBinaryEntry(const QString& entryPath, QByteArray& data) const override;

	[[nodiscard]] bool readTextEntry(const QString& entryPath, QString& data) const override;

	void selectEntryInEntryTree(const QString& entryPath) const override;

private:
	FileViewer* fileViewer;
};

class FileViewer : public QWidget {
	Q_OBJECT;

	friend VPKEditWindowAccess_V2;

public:
	explicit FileViewer(Window* window_, QWidget* parent = nullptr);

	void requestNavigateBack() const;

	void requestNavigateNext() const;

	void displayEntry(const QString& path);

	void displayDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpkpp::PackFile& packFile);

	void addEntry(const vpkpp::Entry& entry, const QString& path) const;

	void removeFile(const QString& path) const;

	void removeDir(const QString& path) const;

	void setSearchQuery(const QString& query) const;

	void setReadOnly(bool readOnly) const;

	void selectSubItemInDir(const QString& name) const;

	[[nodiscard]] bool isDirPreviewVisible() const;

	[[nodiscard]] const QString& getDirPreviewCurrentPath() const;

	void clearContents(bool resetHistory);

	void showTextPreview(const QString& text, const QString& extension);

	void showInfoPreview(const QPixmap& icon, const QString& text);

	void showGenericErrorPreview(const QString& text);

	void showFileLoadErrorPreview();

	void hideAllPreviews();

	[[nodiscard]] NavBar* getNavBar() const {
		return this->navbar;
	}

private:
	Window* window;
	NavBar* navbar;

	VPKEditWindowAccess_V2* packFileAccess_V1;
	QList<QPluginLoader*> previewPlugins;
	DirPreview* dirPreview;
	EmptyPreview* emptyPreview;
	InfoPreview* infoPreview;
	TextPreview* textPreview;
	TexturePreview* texturePreview;
};
