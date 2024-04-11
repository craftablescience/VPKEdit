#pragma once

#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

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

class KeyValuesHighlighter : public QSyntaxHighlighter {
    Q_OBJECT;

public:
    explicit KeyValuesHighlighter(QTextDocument* document = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;
};

class TextPreview : public QPlainTextEdit {
    Q_OBJECT;

public:
    // Reminder if you add a format that should be highlighted to change that list too!
    static inline const QStringList EXTENSIONS {
        ".txt", ".md",                                             // Text
        ".nut", ".lua", ".gm", ".py", ".js", ".ts",                // Scripts
        ".vmf", ".vmm", ".vmx", ".vmt",                            // Assets (1)
        ".vcd", ".fgd", ".qc", ".qci", ".qcx", ".smd",             // Assets (2)
        ".kv", ".kv3", ".res", ".vdf", ".acf", ".bns",             // KeyValues
        ".vbsp", ".rad", ".gi", ".rc", ".lst", ".cfg",             // Valve formats
        ".ini", ".yml", ".yaml", ".toml", ".json",                 // Config
        ".html", ".htm", ".xml", ".css", ".scss", ".sass",         // Web
        "authors", "credits", "license", "readme",                 // Info
        ".gitignore", ".gitattributes", ".gitmodules",             // Git
        ".gd", ".gdshader", ".tscn", ".tres", ".import", ".remap", // Godot
    };

    explicit TextPreview(QWidget* parent = nullptr);

    void setText(const QString& text, const QString& extension);

    [[nodiscard]] int getLineNumberAreaWidth() const;

    void onLineNumberAreaPaintEvent(QPaintEvent* event) const;

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);

    void updateLineNumberArea(const QRect& rect, int dy);

    void highlightCurrentLine();

private:
    LineNumberArea* lineNumberArea;

    KeyValuesHighlighter keyValuesHighlighter;
};
