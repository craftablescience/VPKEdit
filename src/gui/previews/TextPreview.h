#pragma once

#include <QPlainTextEdit>

class TextPreview;

class LineNumberArea : public QWidget {
    Q_OBJECT;

public:
    explicit LineNumberArea(TextPreview* textPreview, QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    TextPreview* preview;
};

class TextPreview : public QPlainTextEdit {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS {
        ".txt", ".md",
        ".vmt", ".vmf", ".vbsp", ".rad",
        ".nut", ".lua", ".gm", ".py", ".js", ".ts",
        ".gi", ".rc", ".lst", ".cfg",
        ".kv", ".kv3", ".res", ".vdf", ".acf",
        ".ini", ".yml", ".yaml", ".toml",
        ".html", ".htm", ".xml", ".css", ".scss", ".sass",
        ".gitignore", "authors", "credits", "license", "readme",
    };

    explicit TextPreview(QWidget* parent = nullptr);

    void setText(const QString& text);

    [[nodiscard]] int getLineNumberAreaWidth();

    void onLineNumberAreaPaintEvent(QPaintEvent* event);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void highlightCurrentLine();

private:
    LineNumberArea* lineNumberArea;
};
