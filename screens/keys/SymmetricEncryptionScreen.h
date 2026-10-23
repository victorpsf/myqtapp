#pragma once

#include "components/forms/ContentPage.h"

class ContentSourceField;
class EncryptedResultCard;
class InputField;
class ProgressStatus;
class QPushButton;
class SelectField;
class TextLabel;

class SymmetricEncryptionScreen : public ContentPage
{
    Q_OBJECT

public:
    explicit SymmetricEncryptionScreen(QWidget *parent = nullptr);

private:
    void updateAlgorithmHint();
    void encrypt();
    void setBusy(bool busy);

    SelectField *m_algorithm;
    TextLabel *m_algorithmHint;
    InputField *m_password;
    InputField *m_confirm;
    ContentSourceField *m_source;
    QPushButton *m_encrypt;
    ProgressStatus *m_progress;
    EncryptedResultCard *m_result;
};
