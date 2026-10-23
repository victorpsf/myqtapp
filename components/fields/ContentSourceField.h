#pragma once

#include <QWidget>

class FilePickerField;
class QRadioButton;
class TextAreaField;

// Escolha entre informar um texto ou um arquivo (botões "Texto" / "Arquivo")
class ContentSourceField : public QWidget
{
    Q_OBJECT

public:
    ContentSourceField(const QString &prompt, const QString &textLabel, const QString &fileLabel,
                       QWidget *parent = nullptr);

    bool isFileMode() const;
    QString text() const;
    QString filePath() const;

    void setPlaceholder(const QString &placeholder);
    void setMonospace(bool monospace);

signals:
    void changed(); // troca de modo, texto editado ou arquivo escolhido

private:
    void updateMode();

    QRadioButton *m_textMode;
    QRadioButton *m_fileMode;
    TextAreaField *m_text;
    FilePickerField *m_file;
};
