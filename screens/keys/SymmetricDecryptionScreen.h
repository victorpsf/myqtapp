#pragma once

#include "components/forms/ContentPage.h"

class ContentSourceField;
class DecryptedResultCard;
class InputField;
class ProgressStatus;
class QPushButton;

class SymmetricDecryptionScreen : public ContentPage
{
    Q_OBJECT

public:
    explicit SymmetricDecryptionScreen(QWidget *parent = nullptr);

private:
    void decrypt();
    void setBusy(bool busy);

    InputField *m_password;
    ContentSourceField *m_source;
    QPushButton *m_decrypt;
    ProgressStatus *m_progress;
    DecryptedResultCard *m_result;
};
