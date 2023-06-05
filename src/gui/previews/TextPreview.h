#pragma once

#include <QTextEdit>

class TextPreview : public QTextEdit {
    Q_OBJECT;

public:
    explicit TextPreview(QWidget* parent = nullptr);
};
