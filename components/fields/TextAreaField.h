#pragma once

#include <QWidget>

class QPlainTextEdit;

// Texto de várias linhas com rótulo
class TextAreaField : public QWidget
{
    Q_OBJECT

public:
    explicit TextAreaField(const QString &label, QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    void clear();

    void setPlaceholder(const QString &placeholder);
    void setReadOnly(bool readOnly);
    void setMonospace(bool monospace);
    void setVisibleLines(int lines);

    QPlainTextEdit *editor() const { return m_editor; }

private:
    QPlainTextEdit *m_editor;
};
