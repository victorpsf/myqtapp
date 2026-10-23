#include "SectionCard.h"

#include <QVBoxLayout>

#include "components/labels/TextLabel.h"

SectionCard::SectionCard(const QString &title, QWidget *parent)
    : QFrame(parent)
    , m_body(new QVBoxLayout(this))
{
    setObjectName("SectionCard");

    m_body->setContentsMargins(20, 18, 20, 20);
    m_body->setSpacing(12);

    if (!title.isEmpty())
        m_body->addWidget(new TextLabel(title, TextLabel::Variant::Section, this));
}

void SectionCard::addWidget(QWidget *widget)
{
    m_body->addWidget(widget);
}

void SectionCard::addLayout(QLayout *layout)
{
    m_body->addLayout(layout);
}
