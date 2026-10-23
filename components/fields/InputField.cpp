#include "InputField.h"

#include <QLabel>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include "components/labels/TextLabel.h"

InputField::InputField(const QString &label, QWidget *parent)
    : QWidget(parent)
    , m_label(new TextLabel(label, TextLabel::Variant::Caption, this))
    , m_input(new QLineEdit(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(m_label);
    layout->addWidget(m_input);

    m_label->setBuddy(m_input);
    m_input->setMinimumHeight(38);

    connect(m_input, &QLineEdit::returnPressed, this, &InputField::submitted);
}

QString InputField::text() const
{
    return m_input->text();
}

void InputField::setText(const QString &text)
{
    m_input->setText(text);
}

void InputField::clear()
{
    m_input->clear();
}

void InputField::setPlaceholder(const QString &placeholder)
{
    m_input->setPlaceholderText(placeholder);
}

void InputField::setPasswordMode(bool enabled)
{
    m_input->setEchoMode(enabled ? QLineEdit::Password : QLineEdit::Normal);
}

void InputField::setMaxLength(int length)
{
    m_input->setMaxLength(length);
}

void InputField::setDigitsOnly(int length)
{
    m_input->setMaxLength(length);
    m_input->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("\\d{0,%1}").arg(length)), m_input));
}

void InputField::setCodeStyle()
{
    QFont font = m_input->font();
    font.setPixelSize(24);
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 8);
    m_input->setFont(font);
    m_input->setAlignment(Qt::AlignCenter);
    m_input->setMinimumHeight(52);
    m_input->setProperty("code", true);
}

void InputField::focusInput()
{
    m_input->setFocus();
    m_input->selectAll();
}
