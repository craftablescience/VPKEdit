#pragma once

#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

class QAction;
class QToolBar;

class FileViewer;
class TextEditor;
class Window;

class LineNumberArea : public QWidget {
	Q_OBJECT;

public:
	explicit LineNumberArea(TextEditor* textEditor, QWidget* parent = nullptr);

	[[nodiscard]] QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	TextEditor* editor;
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

class TextEditor : public QPlainTextEdit {
	Q_OBJECT;

public:
	explicit TextEditor(QWidget* parent = nullptr);

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

class TextPreview : public QWidget {
	Q_OBJECT;

public:
	// Reminder if you add a format that should be highlighted to change that list too!
	static inline const QStringList EXTENSIONS {
		".txt", ".md",															// Text
		".nut", ".lua", ".gm", ".py", ".js", ".ts",					// Scripts
		".vmf", ".vmm", ".vmx", ".vmt",									// Assets (1)
		".vcd", ".fgd", ".qc", ".qci", ".qcx", ".smd",				// Assets (2)
		".kv", ".kv3", ".res", ".vdf", ".acf", ".bns",				// KeyValues (1)
		".zpc", ".zpdata", ".edt",														// KeyValues (2)
		".vbsp", ".rad", ".gi", ".rc", ".lst", ".cfg",				// Valve formats
		".ini", ".yml", ".yaml", ".toml", ".json",						// Config
		".html", ".htm", ".xml", ".css", ".scss", ".sass",			// Web
		"authors", "credits", "license", "readme",							// Info
		".gitignore", ".gitattributes", ".gitmodules",						// Git
		".gd", ".gdshader", ".tscn", ".tres", ".import", ".remap",	// Godot
		".pop",																	// TF2
	};

	TextPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent = nullptr);

	void setText(const QString& text, const QString& extension);

	void setEditing(bool editing) const;

	void setReadOnly(bool readOnly) const;

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	FileViewer* fileViewer;
	Window* window;

	QToolBar* toolbar;
	TextEditor* editor;

	QAction* editAction;
	QAction* saveAction;
	QAction* cancelAction;
};
