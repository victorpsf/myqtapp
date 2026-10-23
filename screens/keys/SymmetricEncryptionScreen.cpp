#include "SymmetricEncryptionScreen.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QtConcurrent/QtConcurrentRun>

#include "components/fields/ContentSourceField.h"
#include "components/fields/InputField.h"
#include "components/fields/SelectField.h"
#include "components/forms/SectionCard.h"
#include "components/labels/ProgressStatus.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "screens/keys/EncryptedResultCard.h"
#include "screens/keys/KeyToolsCommon.h"
#include "security/ContentPackage.h"
#include "security/OpenSsl.h"
#include "security/symmetric/PasswordEncryption.h"

namespace {

struct EncryptionResult
{
    QByteArray data;
    QString error;
};

} // namespace

SymmetricEncryptionScreen::SymmetricEncryptionScreen(QWidget *parent)
    : ContentPage(tr("Criptografia simétrica"),
                  tr("Criptografa um texto ou arquivo com uma senha. A mesma senha será "
                     "necessária para descriptografar."),
                  parent)
    , m_algorithm(new SelectField(tr("Algoritmo")))
    , m_algorithmHint(new TextLabel(QString(), TextLabel::Variant::Caption))
    , m_password(new InputField(tr("Senha")))
    , m_confirm(new InputField(tr("Confirmar senha")))
    , m_source(new ContentSourceField(tr("Criptografar:"), tr("Texto a ser criptografado"),
                                      tr("Arquivo a ser criptografado")))
    , m_encrypt(new QPushButton(tr("Criptografar")))
    , m_progress(new ProgressStatus())
    , m_result(new EncryptedResultCard())
{
    for (const PasswordEncryption::AlgorithmInfo &info : PasswordEncryption::algorithms())
        m_algorithm->addOption(info.name, int(info.algorithm), info.description);

    m_password->setPasswordMode(true);
    m_confirm->setPasswordMode(true);
    m_source->setPlaceholder(tr("Digite ou cole o texto aqui"));

    m_encrypt->setProperty("primary", true);
    m_encrypt->setCursor(Qt::PointingHandCursor);

    auto *settings = new SectionCard(tr("Configuração"));
    settings->addWidget(m_algorithm);
    settings->addWidget(m_algorithmHint);
    settings->addWidget(m_password);
    settings->addWidget(m_confirm);
    settings->addWidget(m_source);

    auto *actions = new QHBoxLayout();
    actions->addWidget(m_progress, 1);
    actions->addStretch();
    actions->addWidget(m_encrypt);
    settings->addLayout(actions);

    addContent(settings);
    addContent(m_result);

    connect(m_algorithm, &SelectField::currentValueChanged, this,
            &SymmetricEncryptionScreen::updateAlgorithmHint);
    connect(m_source, &ContentSourceField::changed, m_result, &EncryptedResultCard::clear);
    connect(m_encrypt, &QPushButton::clicked, this, &SymmetricEncryptionScreen::encrypt);

    updateAlgorithmHint();
}

void SymmetricEncryptionScreen::updateAlgorithmHint()
{
    const auto algorithm = PasswordEncryption::Algorithm(m_algorithm->currentValue().toInt());
    for (const PasswordEncryption::AlgorithmInfo &info : PasswordEncryption::algorithms()) {
        if (info.algorithm == algorithm)
            m_algorithmHint->setText(info.description);
    }
}

void SymmetricEncryptionScreen::encrypt()
{
    const QString password = m_password->text();
    if (password.isEmpty()) {
        Popup::warning(this, tr("Informe a senha."));
        m_password->focusInput();
        return;
    }
    if (password != m_confirm->text()) {
        Popup::warning(this, tr("As senhas não conferem."));
        m_confirm->focusInput();
        return;
    }

    const bool fileMode = m_source->isFileMode();
    const QString filePath = m_source->filePath();
    const QString text = m_source->text();

    if (fileMode && filePath.isEmpty()) {
        Popup::warning(this, tr("Escolha o arquivo a ser criptografado."));
        return;
    }
    if (!fileMode && text.isEmpty()) {
        Popup::warning(this, tr("Informe o texto a ser criptografado."));
        return;
    }

    const auto algorithm = PasswordEncryption::Algorithm(m_algorithm->currentValue().toInt());

    m_result->clear();
    setBusy(true);
    m_progress->start(tr("Derivando a chave da senha e criptografando…"));

    auto *watcher = new QFutureWatcher<EncryptionResult>(this);
    connect(watcher, &QFutureWatcherBase::finished, this,
            [this, watcher, fileMode, filePath, algorithm]() {
                watcher->deleteLater();
                setBusy(false);

                const EncryptionResult result = watcher->result();
                if (!result.error.isEmpty()) {
                    Popup::error(this, result.error);
                    return;
                }

                const QString outputName =
                    fileMode ? KeyTools::encryptedFileName(filePath)
                             : QStringLiteral("texto_%1.enc").arg(KeyTools::timestamp());

                m_result->showResult(tr("<b>%1</b> — %2")
                                         .arg(PasswordEncryption::name(algorithm),
                                              QLocale().formattedDataSize(result.data.size())),
                                     result.data, outputName, !fileMode);
            });

    QByteArray passwordBytes = password.toUtf8();
    const QByteArray textBytes = text.toUtf8();
    watcher->setFuture(QtConcurrent::run(
        [algorithm, passwordBytes, fileMode, filePath, textBytes]() mutable {
            EncryptionResult result;

            ContentPackage::Content content;
            if (fileMode) {
                const std::optional<QByteArray> data =
                    KeyTools::readWholeFile(filePath, &result.error);
                if (!data) {
                    OpenSsl::cleanse(passwordBytes);
                    return result;
                }
                content.fileName = QFileInfo(filePath).fileName();
                content.data = *data;
            } else {
                content.data = textBytes;
            }

            QByteArray plain = ContentPackage::pack(content);
            OpenSsl::cleanse(content.data);

            result.data = PasswordEncryption::encrypt(algorithm, plain, passwordBytes);
            OpenSsl::cleanse(passwordBytes);
            OpenSsl::cleanse(plain);

            if (result.data.isEmpty())
                result.error = QObject::tr("Falha ao criptografar.");
            return result;
        }));
    OpenSsl::cleanse(passwordBytes);
}

void SymmetricEncryptionScreen::setBusy(bool busy)
{
    m_encrypt->setEnabled(!busy);
    m_algorithm->setEnabled(!busy);
    m_password->setEnabled(!busy);
    m_confirm->setEnabled(!busy);
    m_source->setEnabled(!busy);
    if (!busy)
        m_progress->stop();
}
