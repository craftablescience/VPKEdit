#pragma once

#include <typeinfo>
#include <typeindex>

#include <QWidget>

class QLineEdit;
class QToolButton;

namespace vpkedit {

class VPK;

} // namespace vpkedit

class Window;

class NavBar : public QWidget {
	Q_OBJECT;

public:
	explicit NavBar(QWidget* parent = nullptr);

	void setPath(const QString& newPath);

	void clearContents();

signals:
	void pathChanged(const QString& newPath);

private:
	QToolButton* backButton;
	QToolButton* nextButton;
	QToolButton* upButton;
	QLineEdit* currentPath;
};

class FileViewer : public QWidget {
    Q_OBJECT;

public:
    explicit FileViewer(Window* window_, QWidget* parent = nullptr);

    void displayEntry(const QString& path, const vpkedit::VPK& vpk);

    void displayDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpkedit::VPK& vpk);

    void addEntry(const vpkedit::VPK& vpk, const QString& path);

    void removeFile(const QString& path);

    void removeDir(const QString& path);

    void setSearchQuery(const QString& query);

    void selectSubItemInDir(const QString& name) const;

	[[nodiscard]] bool isDirPreviewVisible();

	[[nodiscard]] const QString& getDirPreviewCurrentPath();

    void clearContents();

	template<typename T>
	T* getPreview() {
		return dynamic_cast<T*>(this->previews.at(std::type_index(typeid(T))));
	}

	template<typename T>
	void showPreview() {
		for (const auto [index, widget] : this->previews) {
			widget->hide();
		}
		this->previews.at(std::type_index(typeid(T)))->show();
	}

	template<typename T>
	void hidePreview() const {
		this->previews.at(std::type_index(typeid(T)))->hide();
	}

private:
    Window* window;

	NavBar* navbar;

    std::unordered_map<std::type_index, QWidget*> previews;

	template<typename T, typename... Args>
	T* newPreview(Args... args) {
		auto* preview = new T(std::forward<Args>(args)...);
		this->previews[std::type_index(typeid(T))] = preview;
		return preview;
	}
};
