#pragma once

#include <typeinfo>
#include <typeindex>

#include <QWidget>

class QTextEdit;

namespace vpkedit {

class VPK;

} // namespace vpkedit

class Window;

class FileViewer : public QWidget {
    Q_OBJECT;

public:
    explicit FileViewer(Window* window_, QWidget* parent = nullptr);

    void displayEntry(const QString& path);

    void displayDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpkedit::VPK& vpk);

    void setSearchQuery(const QString& query);

    void selectSubItemInDir(const QString& name);

    void clearContents();

private:
    Window* window;

    std::unordered_map<std::type_index, QWidget*> previews;

    template<typename T, typename... Args>
    T* newPreview(Args... args) {
        auto* preview = new T(std::forward<Args>(args)...);
        this->previews[std::type_index(typeid(T))] = preview;
        return preview;
    }

    template<typename T>
    inline T* getPreview() {
        return dynamic_cast<T*>(this->previews.at(std::type_index(typeid(T))));
    }

    template<typename T>
    void showPreview() {
        for (const auto [index, widget] : this->previews) {
            widget->hide();
        }
        this->previews.at(std::type_index(typeid(T)))->show();
    }
};
