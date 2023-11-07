#include "TextPreview.h"

#include <QPainter>
#include <QStyleOption>
#include <QTextBlock>

constexpr int LINE_NUMBER_AREA_TEXT_MARGIN = 4;

LineNumberArea::LineNumberArea(TextPreview* textPreview, QWidget* parent)
        : QWidget(parent)
        , preview(textPreview) {}

QSize LineNumberArea::sizeHint() const {
    return {this->preview->getLineNumberAreaWidth(), 0};
}

void LineNumberArea::paintEvent(QPaintEvent* event) {
    this->preview->onLineNumberAreaPaintEvent(event);
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
    rule.pattern = QRegularExpression(R"(\[\s*\$[A-Za-z0-9_&|!]+\s*\])");
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

TextPreview::TextPreview(QWidget* parent)
        : QPlainTextEdit(parent)
        , keyValuesHighlighter(nullptr) {
    this->setReadOnly(true);

    this->lineNumberArea = new LineNumberArea(this, this);

    QObject::connect(this, &TextPreview::blockCountChanged, this, &TextPreview::updateLineNumberAreaWidth);
    QObject::connect(this, &TextPreview::updateRequest, this, &TextPreview::updateLineNumberArea);
    QObject::connect(this, &TextPreview::cursorPositionChanged, this, &TextPreview::highlightCurrentLine);

    this->updateLineNumberAreaWidth(0);
    this->highlightCurrentLine();

    // Set font
    QFont monospace;
    monospace.setFamily("Lucida Console");
    monospace.setStyleHint(QFont::Monospace);
    this->setFont(monospace);
    this->lineNumberArea->setFont(monospace);
}

void TextPreview::setText(const QString& text, const QString& extension) {
    this->document()->setPlainText(text);

    // Copied from the header
    const QStringList keyValuesLikeFormats = {
            ".vmf", ".vmm", ".vmx", ".vmt",                // Assets (1)
            ".vcd", ".fgd", ".qc", ".smd",                 // Assets (2)
            ".kv", ".kv3", ".res", ".vdf", ".acf",         // KeyValues
            ".vbsp", ".rad", ".gi", ".rc", ".lst", ".cfg", // Valve formats
    };
    if (keyValuesLikeFormats.contains(extension)) {
        this->keyValuesHighlighter.setDocument(this->document());
    } else {
        this->keyValuesHighlighter.setDocument(nullptr);
    }
}

int TextPreview::getLineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, this->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = (LINE_NUMBER_AREA_TEXT_MARGIN * 2) + (this->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits);
    return space;
}

void TextPreview::onLineNumberAreaPaintEvent(QPaintEvent* event) {
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

void TextPreview::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = this->contentsRect();
    this->lineNumberArea->setGeometry({cr.left(), cr.top(), this->getLineNumberAreaWidth(), cr.height()});
}

void TextPreview::updateLineNumberAreaWidth(int /*newBlockCount*/) {
    this->setViewportMargins(this->getLineNumberAreaWidth(), 0, 0, 0);
}

void TextPreview::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy) {
        this->lineNumberArea->scroll(0, dy);
    } else {
        this->lineNumberArea->update(0, rect.y(), this->lineNumberArea->width(), rect.height());
    }

    if (rect.contains(this->viewport()->rect())) {
        this->updateLineNumberAreaWidth(0);
    }
}

void TextPreview::highlightCurrentLine() {
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
