#include "TextLabel.h"

#include <QStyle>

TextLabel::TextLabel(const QString &text, Variant variant, QWidget *parent)
    : QLabel(text, parent)
    , m_variant(variant)
{
    setWordWrap(true);
    setVariant(variant);
}

void TextLabel::setVariant(Variant variant)
{
    m_variant = variant;

    const char *name = "body";
    switch (variant) {
    case Variant::Title:    name = "title"; break;
    case Variant::Section:  name = "section"; break;
    case Variant::Subtitle: name = "subtitle"; break;
    case Variant::Body:     name = "body"; break;
    case Variant::Caption:  name = "caption"; break;
    }

    setProperty("variant", name);
    style()->unpolish(this);
    style()->polish(this);
}
