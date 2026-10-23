#pragma once

#include <QVariant>
#include <QWidget>

class QComboBox;

// Lista de opções com rótulo
class SelectField : public QWidget
{
    Q_OBJECT

public:
    explicit SelectField(const QString &label, QWidget *parent = nullptr);

    void addOption(const QString &text, const QVariant &value, const QString &toolTip = QString());
    void clearOptions();

    QVariant currentValue() const;
    void setCurrentValue(const QVariant &value);
    QString currentText() const;

    QComboBox *comboBox() const { return m_combo; }

signals:
    void currentValueChanged(const QVariant &value);

private:
    QComboBox *m_combo;
};
