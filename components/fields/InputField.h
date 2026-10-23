#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;

// Campo de texto com rótulo
class InputField : public QWidget
{
    Q_OBJECT

public:
    explicit InputField(const QString &label, QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    void clear();

    void setPlaceholder(const QString &placeholder);
    void setPasswordMode(bool enabled);
    void setMaxLength(int length);
    void setDigitsOnly(int length);
    void setCodeStyle(); // fonte grande, centralizado, dígitos espaçados
    void focusInput();

    QLineEdit *lineEdit() const { return m_input; }

signals:
    void submitted();

private:
    QLabel *m_label;
    QLineEdit *m_input;
};
