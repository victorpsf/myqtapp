#pragma once

#include <QByteArray>

#include "components/forms/ContentPage.h"

class FilePickerField;
class ProgressStatus;
class QLineEdit;
class QPushButton;
class QRadioButton;
class SectionCard;
class SelectField;
class TextAreaField;
class TextLabel;

class HashScreen : public ContentPage
{
    Q_OBJECT

public:
    explicit HashScreen(QWidget *parent = nullptr);

private:
    bool isFileMode() const;
    void updateMode();
    void generate();
    void showResult(const QByteArray &digest, const QString &source);
    void setBusy(bool busy);

    QWidget *createOutputRow(QLineEdit *output);

    SelectField *m_algorithm;
    TextLabel *m_weakWarning;
    QRadioButton *m_textMode;
    QRadioButton *m_fileMode;
    TextAreaField *m_text;
    FilePickerField *m_file;
    QPushButton *m_generate;
    ProgressStatus *m_progress;

    SectionCard *m_result;
    TextLabel *m_summary;
    QLineEdit *m_hex;
    QLineEdit *m_base64;
};
