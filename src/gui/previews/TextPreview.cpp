#include "TextPreview.h"

TextPreview::TextPreview(QWidget* parent)
        : QTextEdit(parent) {
    this->setReadOnly(true);

    QFont monospace;
    monospace.setFamily("Lucida Console");
    monospace.setStyleHint(QFont::Monospace);
    this->setFont(monospace);
}
