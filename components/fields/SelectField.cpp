#include "SelectField.h"

#include <QComboBox>
#include <QVBoxLayout>

#include "components/labels/TextLabel.h"

SelectField::SelectField(const QString &label, QWidget *parent)
    : QWidget(parent)
    , m_combo(new QComboBox(this))
{
    auto *title = new TextLabel(label, TextLabel::Variant::Caption, this);
    title->setBuddy(m_combo);
    m_combo->setMinimumHeight(38);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(title);
    layout->addWidget(m_combo);

    connect(m_combo, &QComboBox::currentIndexChanged, this,
            [this]() { emit currentValueChanged(currentValue()); });
}

void SelectField::addOption(const QString &text, const QVariant &value, const QString &toolTip)
{
    m_combo->addItem(text, value);
    if (!toolTip.isEmpty())
        m_combo->setItemData(m_combo->count() - 1, toolTip, Qt::ToolTipRole);
}

void SelectField::clearOptions()
{
    m_combo->clear();
}

QVariant SelectField::currentValue() const
{
    return m_combo->currentData();
}

void SelectField::setCurrentValue(const QVariant &value)
{
    const int index = m_combo->findData(value);
    if (index >= 0)
        m_combo->setCurrentIndex(index);
}

QString SelectField::currentText() const
{
    return m_combo->currentText();
}
