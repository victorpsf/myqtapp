#pragma once

#include <atomic>
#include <memory>

#include "components/forms/ContentPage.h"
#include "security/asymmetric/AsymmetricKeyGenerator.h"

class InputField;
class ProgressStatus;
class QPushButton;
class SectionCard;
class SelectField;
class TextAreaField;
class TextLabel;

class AsymmetricKeyScreen : public ContentPage
{
    Q_OBJECT

public:
    explicit AsymmetricKeyScreen(QWidget *parent = nullptr);
    ~AsymmetricKeyScreen() override;

private:
    AsymmetricKeyGenerator::Options options() const;
    void updateAlgorithmOptions();
    void generate();
    void cancel();
    void save();
    void setBusy(bool busy);
    void clearResult();

    SelectField *m_algorithm;
    TextLabel *m_algorithmHint;
    SelectField *m_rsaSize;
    SelectField *m_curve;
    InputField *m_password;
    InputField *m_confirm;
    QPushButton *m_generate;
    QPushButton *m_cancel;
    ProgressStatus *m_progress;

    SectionCard *m_result;
    TextLabel *m_summary;
    TextAreaField *m_publicKey;
    QPushButton *m_save;

    AsymmetricKeyGenerator::KeyPair m_keys;
    QString m_filePrefix;
    std::shared_ptr<std::atomic_bool> m_cancelFlag;
};
