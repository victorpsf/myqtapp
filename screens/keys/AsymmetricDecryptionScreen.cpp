#include "AsymmetricDecryptionScreen.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QPushButton>
#include <QtConcurrent/QtConcurrentRun>

#include "components/fields/ContentSourceField.h"
#include "components/fields/FilePickerField.h"
#include "components/fields/InputField.h"
#include "components/forms/SectionCard.h"
#include "components/labels/ProgressStatus.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "screens/keys/DecryptedResultCard.h"
#include "screens/keys/KeyToolsCommon.h"
#include "security/ContentPackage.h"
#include "security/OpenSsl.h"
#include "security/asymmetric/PublicKeyEncryption.h"

namespace {

struct DecryptionResult
{
    ContentPackage::Content content;
    QString error;
};

} // namespace

AsymmetricDecryptionScreen::AsymmetricDecryptionScreen(QWidget *parent)
    : ContentPage(tr("Descriptografar com chave privada"),
                  tr("Descriptografa um texto ou arquivo gerado em \"Criptografar (assimétrica)\" "
                     "usando a chave privada (.pem) correspondente à chave pública usada."),
                  parent)
    , m_keyFile(new FilePickerField(tr("Chave privada (.pem)")))
    , m_keyInfo(new TextLabel(QString(), TextLabel::Variant::Caption))
    , m_password(new InputField(tr("Senha da chave privada")))
    , m_source(new ContentSourceField(tr("Descriptografar:"),
                                      tr("Conteúdo criptografado (base64)"),
                                      tr("Arquivo criptografado")))
    , m_decrypt(new QPushButton(tr("Descriptografar")))
    , m_progress(new ProgressStatus())
    , m_result(new DecryptedResultCard())
{
    m_keyInfo->setText(tr("Use o arquivo *_private_key.pem gerado em \"Criar chaves (assimétrica)\". "
                          "A chave é lida só no momento de descriptografar e não fica guardada."));
    m_password->setPasswordMode(true);
    m_password->setPlaceholder(tr("Deixe vazio se a chave não tem senha"));
    m_source->setPlaceholder(tr("Cole aqui o base64 gerado na criptografia"));
    m_source->setMonospace(true);

    m_decrypt->setProperty("primary", true);
    m_decrypt->setCursor(Qt::PointingHandCursor);

    auto *keyCard = new SectionCard(tr("Chave"));
    keyCard->addWidget(m_keyFile);
    keyCard->addWidget(m_keyInfo);
    keyCard->addWidget(m_password);

    auto *contentCard = new SectionCard(tr("Conteúdo"));
    contentCard->addWidget(m_source);

    auto *actions = new QHBoxLayout();
    actions->addWidget(m_progress, 1);
    actions->addStretch();
    actions->addWidget(m_decrypt);
    contentCard->addLayout(actions);

    addContent(keyCard);
    addContent(contentCard);
    addContent(m_result);

    connect(m_keyFile, &FilePickerField::fileSelected, this,
            &AsymmetricDecryptionScreen::inspectKey);
    connect(m_source, &ContentSourceField::changed, m_result, &DecryptedResultCard::clear);
    connect(m_decrypt, &QPushButton::clicked, this, &AsymmetricDecryptionScreen::decrypt);
}

void AsymmetricDecryptionScreen::inspectKey(const QString &path)
{
    m_result->clear();

    QString error;
    std::optional<QByteArray> pem = KeyTools::readKeyFile(path, &error);
    if (!pem) {
        m_keyInfo->setText(error);
        return;
    }

    const PublicKeyEncryption::KeyInfo info = PublicKeyEncryption::inspect(*pem);
    OpenSsl::cleanse(*pem);

    if (!info.error.isEmpty())
        m_keyInfo->setText(info.error);
    else if (!info.isPrivate)
        m_keyInfo->setText(tr("Este arquivo é uma chave pública. Para descriptografar escolha a "
                              "chave privada (*_private_key.pem)."));
    else if (info.isEncrypted)
        m_keyInfo->setText(tr("✔ %1 — informe a senha abaixo.").arg(info.description));
    else
        m_keyInfo->setText(tr("✔ Chave privada %1 (sem senha)").arg(info.description));
}

void AsymmetricDecryptionScreen::decrypt()
{
    const QString keyPath = m_keyFile->filePath();
    if (keyPath.isEmpty()) {
        Popup::warning(this, tr("Escolha a chave privada."));
        return;
    }

    const bool fileMode = m_source->isFileMode();
    const QString filePath = m_source->filePath();

    QByteArray encrypted;
    if (fileMode && filePath.isEmpty()) {
        Popup::warning(this, tr("Escolha o arquivo criptografado."));
        return;
    }
    if (!fileMode) {
        const std::optional<QByteArray> decoded = KeyTools::decodeBase64(m_source->text());
        if (!decoded) {
            Popup::warning(this, tr("Cole o conteúdo criptografado em base64."));
            return;
        }
        encrypted = *decoded;
    }

    m_result->clear();
    setBusy(true);
    m_progress->start(tr("Descriptografando…"));

    auto *watcher = new QFutureWatcher<DecryptionResult>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, fileMode, filePath]() {
        watcher->deleteLater();
        setBusy(false);

        const DecryptionResult result = watcher->result();
        if (!result.error.isEmpty()) {
            Popup::error(this, result.error);
            return;
        }

        const QString fallbackName =
            fileMode ? QStringLiteral("descriptografado_%1").arg(QFileInfo(filePath).fileName())
                     : QStringLiteral("descriptografado_%1.bin").arg(KeyTools::timestamp());
        m_result->showContent(tr("<b>Descriptografado com sucesso</b>"), result.content,
                              fallbackName);
    });

    QByteArray passwordBytes = m_password->text().toUtf8();
    watcher->setFuture(QtConcurrent::run(
        [keyPath, passwordBytes, fileMode, filePath, encrypted]() mutable {
            DecryptionResult result;

            std::optional<QByteArray> privatePem = KeyTools::readKeyFile(keyPath, &result.error);
            if (!privatePem) {
                OpenSsl::cleanse(passwordBytes);
                return result;
            }

            if (fileMode) {
                const std::optional<QByteArray> data =
                    KeyTools::readWholeFile(filePath, &result.error);
                if (!data) {
                    OpenSsl::cleanse(*privatePem);
                    OpenSsl::cleanse(passwordBytes);
                    return result;
                }
                encrypted = *data;
            }

            PublicKeyEncryption::Result decrypted =
                PublicKeyEncryption::decrypt(*privatePem, passwordBytes, encrypted);
            OpenSsl::cleanse(*privatePem);
            OpenSsl::cleanse(passwordBytes);

            if (!decrypted.isSuccess()) {
                result.error = decrypted.error;
                return result;
            }

            result.content = ContentPackage::unpack(decrypted.data);
            OpenSsl::cleanse(decrypted.data);
            return result;
        }));
    OpenSsl::cleanse(passwordBytes);
}

void AsymmetricDecryptionScreen::setBusy(bool busy)
{
    m_decrypt->setEnabled(!busy);
    m_keyFile->setEnabled(!busy);
    m_password->setEnabled(!busy);
    m_source->setEnabled(!busy);
    if (!busy)
        m_progress->stop();
}
