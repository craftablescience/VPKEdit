#include "FileViewer.h"

#include <QHBoxLayout>
#include <QTextEdit>

#include "previews/VTFPreview.h"
#include "Window.h"

using namespace vpktool;

FileViewer::FileViewer(Window* window_, QWidget* parent)
        : QWidget(parent)
        , window(window_) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    this->textEdit = new QTextEdit(this);
    this->textEdit->setReadOnly(true);

    QFont monospace;
    monospace.setFamily("Lucida Console");
    monospace.setStyleHint(QFont::Monospace);
    this->textEdit->setFont(monospace);

    layout->addWidget(textEdit);

    this->image = new VTFPreview(this);
    layout->addWidget(this->image);

    this->clearContents();
    this->setTextEditVisible();
}

void FileViewer::displayEntry(const QString& path) {
    this->clearContents();
    if (path.endsWith(".txt") ||
        path.endsWith(".md")  ||
        path.endsWith(".gi")  ||
        path.endsWith(".res") ||
        path.endsWith(".nut") ||
        path.endsWith(".lua") ||
        path.endsWith(".cfg") ||
        path.endsWith(".ini") ||
        path.endsWith(".kv")  ||
        path.endsWith(".kv3") ||
        path.endsWith(".vmf") || // hey you never know
        path.endsWith(".vmt")) {
        // It's text
        this->textEdit->setText(this->window->readTextEntry(path));
        this->setTextEditVisible();
    } else if (path.endsWith(".vtf")) {
        // VTF (image)
        this->image->setImage(this->window->readBinaryEntry(path));
        this->setImageVisible();
    }
}

void FileViewer::clearContents() {
    this->textEdit->setText("");
    this->setTextEditVisible();
}

void FileViewer::setTextEditVisible() {
    this->textEdit->show();
    this->image->hide();
}

void FileViewer::setImageVisible() {
    this->image->show();
    this->textEdit->hide();
}
