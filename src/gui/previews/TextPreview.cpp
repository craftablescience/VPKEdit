#include "TextPreview.h"

#include <QPainter>
#include <QScrollBar>
#include <QStyleOption>
#include <QTextBlock>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "../FileViewer.h"
#include "../Window.h"

constexpr int LINE_NUMBER_AREA_TEXT_MARGIN = 4;

LineNumberArea::LineNumberArea(TextEditor* textEditor, QWidget* parent)
		: QWidget(parent)
		, editor(textEditor) {}

QSize LineNumberArea::sizeHint() const {
	return {this->editor->getLineNumberAreaWidth(), 0};
}

void LineNumberArea::paintEvent(QPaintEvent* event) {
	this->editor->onLineNumberAreaPaintEvent(event);
}

KeyValuesHighlighter::KeyValuesHighlighter(QTextDocument* document)
		: QSyntaxHighlighter(document) {
	HighlightingRule rule;

	QTextCharFormat pragmaFormat;
	pragmaFormat.setForeground(Qt::darkMagenta);
	rule.pattern = QRegularExpression("\\b#[A-Za-z_]+");
	rule.format = pragmaFormat;
	this->highlightingRules.append(rule);

	QTextCharFormat variableFormat;
	pragmaFormat.setForeground(Qt::magenta);
	rule.pattern = QRegularExpression(R"(\[\s*!?\$[A-Za-z0-9_&|!]+\s*\])");
	rule.format = pragmaFormat;
	this->highlightingRules.append(rule);

	QTextCharFormat quotationFormat;
	quotationFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegularExpression("\".*\"");
	rule.format = quotationFormat;
	this->highlightingRules.append(rule);

	QTextCharFormat singleLineCommentFormat;
	singleLineCommentFormat.setForeground(Qt::gray);
	rule.pattern = QRegularExpression("//[^\n]*");
	rule.format = singleLineCommentFormat;
	this->highlightingRules.append(rule);
}

void KeyValuesHighlighter::highlightBlock(const QString& text) {
	for (const auto& rule : this->highlightingRules) {
		auto matchIterator = rule.pattern.globalMatch(text);
		while (matchIterator.hasNext()) {
			QRegularExpressionMatch match = matchIterator.next();
			this->setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), rule.format);
		}
	}
}

TextEditor::TextEditor(QWidget* parent)
		: QPlainTextEdit(parent)
		, keyValuesHighlighter(nullptr) {
	this->lineNumberArea = new LineNumberArea(this, this);

	QObject::connect(this, &TextEditor::blockCountChanged, this, &TextEditor::updateLineNumberAreaWidth);
	QObject::connect(this, &TextEditor::updateRequest, this, &TextEditor::updateLineNumberArea);
	QObject::connect(this, &TextEditor::cursorPositionChanged, this, &TextEditor::highlightCurrentLine);

	this->updateLineNumberAreaWidth(0);
	this->highlightCurrentLine();

	// Set font
	QFont monospace;
	monospace.setFamily("Lucida Console");
	monospace.setStyleHint(QFont::Monospace);
	this->setFont(monospace);
	this->lineNumberArea->setFont(monospace);
}

void TextEditor::setText(const QString& text, const QString& extension) {
	this->document()->setPlainText(text);

	// Copied from the header
	const QStringList keyValuesLikeFormats = {
		".vmf", ".vmm", ".vmx", ".vmt",
		".vcd", ".fgd", ".qc", ".smd",
		".kv", ".kv3", ".res", ".vdf", ".acf", ".bns",
		".zpc", ".zpdata", ".edt",
		".vbsp", ".rad", ".gi", ".rc", ".lst", ".cfg",
		".scr", ".dlg", ".lip",
	};
	if (keyValuesLikeFormats.contains(extension)) {
		this->keyValuesHighlighter.setDocument(this->document());
	} else {
		this->keyValuesHighlighter.setDocument(nullptr);
	}
}

int TextEditor::getLineNumberAreaWidth() const {
	int digits = 1;
	int max = qMax(1, this->blockCount());
	while (max >= 10) {
		max /= 10;
		++digits;
	}
	int space = (LINE_NUMBER_AREA_TEXT_MARGIN * 2) + (this->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits);
	return space;
}

void TextEditor::onLineNumberAreaPaintEvent(QPaintEvent* event) const {
	QStyleOption opt;
	opt.initFrom(this);

	QPainter painter(this->lineNumberArea);
	painter.fillRect(event->rect(), opt.palette.color(QPalette::ColorRole::Button));

	QTextBlock block = this->firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = qRound(this->blockBoundingGeometry(block).translated(this->contentOffset()).top());
	int bottom = top + qRound(this->blockBoundingRect(block).height());

	while (block.isValid() && top <= event->rect().bottom()) {
		if (block.isVisible() && bottom >= event->rect().top()) {
			QString number = QString::number(blockNumber + 1);
			painter.setPen(opt.palette.color(QPalette::ColorRole::Text));
			painter.drawText(0, top, this->lineNumberArea->width() - LINE_NUMBER_AREA_TEXT_MARGIN, this->fontMetrics().height(), Qt::AlignRight, number);
		}
		block = block.next();
		top = bottom;
		bottom = top + qRound(this->blockBoundingRect(block).height());
		blockNumber++;
	}
}

void TextEditor::resizeEvent(QResizeEvent* event) {
	QPlainTextEdit::resizeEvent(event);

	QRect cr = this->contentsRect();
	this->lineNumberArea->setGeometry({cr.left(), cr.top(), this->getLineNumberAreaWidth(), cr.height()});
}

void TextEditor::updateLineNumberAreaWidth(int /*newBlockCount*/) {
	this->setViewportMargins(this->getLineNumberAreaWidth(), 0, 0, 0);
}

void TextEditor::updateLineNumberArea(const QRect& rect, int dy) {
	if (dy) {
		this->lineNumberArea->scroll(0, dy);
	} else {
		this->lineNumberArea->update(0, rect.y(), this->lineNumberArea->width(), rect.height());
	}

	if (rect.contains(this->viewport()->rect())) {
		this->updateLineNumberAreaWidth(0);
	}
}

void TextEditor::highlightCurrentLine() {
	QList<QTextEdit::ExtraSelection> extraSelections;
	if (!this->isReadOnly()) {
		QTextEdit::ExtraSelection selection;

		auto color = QColor(Qt::gray);
		color.setAlphaF(0.1f);

		selection.format.setBackground(color);
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = textCursor();
		selection.cursor.clearSelection();
		extraSelections.append(selection);
	}
	this->setExtraSelections(extraSelections);
}

TextPreview::TextPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent)
		: QWidget(parent)
		, fileViewer(fileViewer_)
		, window(window_) {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);

	this->toolbar = new QToolBar(this);
	this->toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	auto* spacer = new QWidget(this->toolbar);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	this->toolbar->addWidget(spacer);

	this->editor = new TextEditor(this);
	layout->addWidget(this->editor);

	this->editAction = this->toolbar->addAction(this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Edit"));
	QObject::connect(this->editAction, &QAction::triggered, this, [this] {
		this->setEditing(true);
	});

	this->saveAction = this->toolbar->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save"));
	QObject::connect(this->saveAction, &QAction::triggered, this, [this] {
		this->setEditing(false);
		this->window->editFileContents(this->fileViewer->getNavBar()->path(), this->editor->toPlainText());
	});

	this->cancelAction = this->toolbar->addAction(this->style()->standardIcon(QStyle::SP_BrowserReload), tr("Cancel"));
	QObject::connect(this->cancelAction, &QAction::triggered, this, [this] {
		this->setEditing(false);

		// hack: reselect the entry to reload its contents
		this->window->selectEntryInEntryTree(this->fileViewer->getNavBar()->path());
	});

	this->toolbar->raise();

	// Do this manually so we're not calling Window::freezeActions
	this->editor->setReadOnly(true);
}

void TextPreview::setText(const QString& text, const QString& extension) {
	this->editor->setText(text, extension);
	this->recomputeToolbarVisibility();
}

void TextPreview::setEditing(bool editing) const {
	this->editor->setReadOnly(!editing);
	this->editAction->setVisible(!editing);
	this->saveAction->setVisible(editing);
	this->cancelAction->setVisible(editing);
	this->fileViewer->getNavBar()->setDisabled(editing);
	this->window->freezeActions(editing, true, false);
	this->recomputeToolbarVisibility();
}

void TextPreview::setReadOnly(bool readOnly) const {
	this->editAction->setVisible(!readOnly);
	this->saveAction->setVisible(false);
	this->cancelAction->setVisible(false);
	this->recomputeToolbarVisibility();
}

void TextPreview::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	auto width = this->width();
	if (const auto* scrollbar = this->editor->verticalScrollBar()) {
		width -= scrollbar->sizeHint().width();
	}
	this->toolbar->setFixedSize(width, 64);
	this->recomputeToolbarVisibility();
}

void TextPreview::recomputeToolbarVisibility() const {
	// must be delayed a single frame
	QTimer::singleShot(0, this, [this] {
		QRegion reg;
		reg += this->toolbar->actionGeometry(this->editAction);
		reg += this->toolbar->actionGeometry(this->saveAction);
		reg += this->toolbar->actionGeometry(this->cancelAction);
		this->toolbar->setMask(reg);
	});
}
