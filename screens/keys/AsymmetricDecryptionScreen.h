#pragma once

#include "components/forms/ContentPage.h"

class ContentSourceField;
class DecryptedResultCard;
class FilePickerField;
class InputField;
class ProgressStatus;
class QPushButton;
class TextLabel;

class AsymmetricDecryptionScreen : public ContentPage
{
    Q_OBJECT

public:
    explicit AsymmetricDecryptionScreen(QWidget *parent = nullptr);

private:
    void inspectKey(const QString &path);
    void decrypt();
    void setBusy(bool busy);

    FilePickerField *m_keyFile;
    TextLabel *m_keyInfo;
    InputField *m_password;
    ContentSourceField *m_source;
    QPushButton *m_decrypt;
    ProgressStatus *m_progress;
    DecryptedResultCard *m_result;
};
