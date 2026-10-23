#include "TextAreaField.h"

#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "components/labels/TextLabel.h"

TextAreaField::TextAreaField(const QString &label, QWidget *parent)
    : QWidget(parent)
    , m_editor(new QPlainTextEdit(this))
{
    auto *title = new TextLabel(label, TextLabel::Variant::Caption, this);
    title->setBuddy(m_editor);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(title);
    layout->addWidget(m_editor);

    setVisibleLines(5);
}

QString TextAreaField::text() const
{
    return m_editor->toPlainText();
}

void TextAreaField::setText(const QString &text)
{
    m_editor->setPlainText(text);
}

void TextAreaField::clear()
{
    m_editor->clear();
}

void TextAreaField::setPlaceholder(const QString &placeholder)
{
    m_editor->setPlaceholderText(placeholder);
}

void TextAreaField::setReadOnly(bool readOnly)
{
    m_editor->setReadOnly(readOnly);
}

void TextAreaField::setMonospace(bool monospace)
{
    m_editor->setFont(monospace ? QFontDatabase::systemFont(QFontDatabase::FixedFont) : font());
}

void TextAreaField::setVisibleLines(int lines)
{
    const int height = m_editor->fontMetrics().lineSpacing() * lines + 20;
    m_editor->setMinimumHeight(height);
    m_editor->setMaximumHeight(height * 2);
}
