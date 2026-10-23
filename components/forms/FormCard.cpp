#include "FormCard.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include "components/labels/TextLabel.h"

FormCard::FormCard(const QString &title, const QString &subtitle, QWidget *parent)
    : QFrame(parent)
    , m_title(new TextLabel(title, TextLabel::Variant::Title, this))
    , m_subtitle(new TextLabel(subtitle, TextLabel::Variant::Subtitle, this))
    , m_fields(new QVBoxLayout())
    , m_buttons(new QHBoxLayout())
{
    setObjectName("FormCard");
    setFixedWidth(380);

    m_subtitle->setVisible(!subtitle.isEmpty());

    m_fields->setSpacing(12);
    m_buttons->setSpacing(8);
    m_buttons->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(16);
    layout->addWidget(m_title);
    layout->addWidget(m_subtitle);
    layout->addLayout(m_fields);
    layout->addSpacing(4);
    layout->addLayout(m_buttons);
}

void FormCard::setTitle(const QString &title)
{
    m_title->setText(title);
}

void FormCard::setSubtitle(const QString &subtitle)
{
    m_subtitle->setText(subtitle);
    m_subtitle->setVisible(!subtitle.isEmpty());
}

void FormCard::addField(QWidget *field)
{
    m_fields->addWidget(field);
}

QPushButton *FormCard::addButton(const QString &text, bool primary)
{
    auto *button = new QPushButton(text, this);
    button->setProperty("primary", primary);
    button->setCursor(Qt::PointingHandCursor);
    button->setAutoDefault(false);
    m_buttons->addWidget(button);
    m_buttonList.append(button);
    return button;
}

void FormCard::setBusy(bool busy)
{
    for (QPushButton *button : m_buttonList)
        button->setEnabled(!busy);

    if (busy)
        setCursor(Qt::BusyCursor);
    else
        unsetCursor();
}

QWidget *FormCard::centered(FormCard *card, QWidget *parent)
{
    // Sem Qt::AlignHCenter: com alinhamento o layout ignora o heightForWidth
    // dos textos com quebra de linha e o cartão fica espremido.
    auto *row = new QHBoxLayout();
    row->addStretch();
    row->addWidget(card);
    row->addStretch();

    auto *wrapper = new QWidget(parent);
    auto *layout = new QVBoxLayout(wrapper);
    layout->addStretch();
    layout->addLayout(row);
    layout->addStretch();
    return wrapper;
}
