#pragma once

#include "components/forms/ContentPage.h"

class ContentSourceField;
class EncryptedResultCard;
class FilePickerField;
class ProgressStatus;
class QPushButton;
class TextLabel;

class AsymmetricEncryptionScreen : public ContentPage
{
    Q_OBJECT

public:
    explicit AsymmetricEncryptionScreen(QWidget *parent = nullptr);

private:
    void loadKey(const QString &path);
    void encrypt();
    void setBusy(bool busy);

    FilePickerField *m_keyFile;
    TextLabel *m_keyInfo;
    ContentSourceField *m_source;
    QPushButton *m_encrypt;
    ProgressStatus *m_progress;
    EncryptedResultCard *m_result;

    QByteArray m_publicPem;
    QString m_keyDescription;
};
