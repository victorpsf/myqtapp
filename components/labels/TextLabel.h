#pragma once

#include <QLabel>

class TextLabel : public QLabel
{
    Q_OBJECT

public:
    enum class Variant { Title, Section, Subtitle, Body, Caption };

    explicit TextLabel(const QString &text = QString(), Variant variant = Variant::Body,
                       QWidget *parent = nullptr);

    void setVariant(Variant variant);
    Variant variant() const { return m_variant; }

private:
    Variant m_variant;
};
