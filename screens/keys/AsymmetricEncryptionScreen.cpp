#include "AsymmetricEncryptionScreen.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QtConcurrent/QtConcurrentRun>

#include "components/fields/ContentSourceField.h"
#include "components/fields/FilePickerField.h"
#include "components/forms/SectionCard.h"
#include "components/labels/ProgressStatus.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "screens/keys/EncryptedResultCard.h"
#include "screens/keys/KeyToolsCommon.h"
#include "security/ContentPackage.h"
#include "security/OpenSsl.h"
#include "security/asymmetric/PublicKeyEncryption.h"

AsymmetricEncryptionScreen::AsymmetricEncryptionScreen(QWidget *parent)
    : ContentPage(tr("Criptografar com chave pública"),
                  tr("Criptografa um texto ou arquivo com a chave pública (.pem) do destinatário. "
                     "Só quem tem a chave privada correspondente consegue descriptografar."),
                  parent)
    , m_keyFile(new FilePickerField(tr("Chave pública (.pem)")))
    , m_keyInfo(new TextLabel(QString(), TextLabel::Variant::Caption))
    , m_source(new ContentSourceField(tr("Criptografar:"), tr("Texto a ser criptografado"),
                                      tr("Arquivo a ser criptografado")))
    , m_encrypt(new QPushButton(tr("Criptografar")))
    , m_progress(new ProgressStatus())
    , m_result(new EncryptedResultCard())
{
    m_keyInfo->setText(tr("Use o arquivo *_public_key.pem gerado em \"Criar chaves (assimétrica)\". "
                          "Aceita RSA (1024 bits ou mais) e curva elíptica (ECDSA)."));
    m_source->setPlaceholder(tr("Digite ou cole o texto aqui"));

    m_encrypt->setProperty("primary", true);
    m_encrypt->setCursor(Qt::PointingHandCursor);

    auto *keyCard = new SectionCard(tr("Chave"));
    keyCard->addWidget(m_keyFile);
    keyCard->addWidget(m_keyInfo);

    auto *contentCard = new SectionCard(tr("Conteúdo"));
    contentCard->addWidget(m_source);

    auto *actions = new QHBoxLayout();
    actions->addWidget(m_progress, 1);
    actions->addStretch();
    actions->addWidget(m_encrypt);
    contentCard->addLayout(actions);

    addContent(keyCard);
    addContent(contentCard);
    addContent(m_result);

    connect(m_keyFile, &FilePickerField::fileSelected, this, &AsymmetricEncryptionScreen::loadKey);
    connect(m_source, &ContentSourceField::changed, m_result, &EncryptedResultCard::clear);
    connect(m_encrypt, &QPushButton::clicked, this, &AsymmetricEncryptionScreen::encrypt);
}

void AsymmetricEncryptionScreen::loadKey(const QString &path)
{
    m_publicPem.clear();
    m_keyDescription.clear();
    m_result->clear();

    QString error;
    const std::optional<QByteArray> pem = KeyTools::readKeyFile(path, &error);
    const PublicKeyEncryption::KeyInfo info =
        pem ? PublicKeyEncryption::inspect(*pem) : PublicKeyEncryption::KeyInfo();

    if (!pem)
        m_keyInfo->setText(error);
    else if (!info.error.isEmpty())
        m_keyInfo->setText(info.error);
    else if (info.isPrivate)
        m_keyInfo->setText(tr("Este arquivo é uma chave privada. Para criptografar escolha a chave "
                              "pública (*_public_key.pem); a privada fica só com o destinatário."));
    else if (!info.canEncrypt)
        m_keyInfo->setText(tr("%1: esta chave não serve para criptografia (Ed25519/Ed448 são só "
                              "de assinatura; RSA precisa de 1024 bits ou mais).")
                               .arg(info.description));
    else {
        m_publicPem = *pem;
        m_keyDescription = info.description;
        m_keyInfo->setText(tr("✔ Chave pública %1").arg(info.description));
    }
}

void AsymmetricEncryptionScreen::encrypt()
{
    if (m_publicPem.isEmpty()) {
        Popup::warning(this, tr("Escolha uma chave pública válida."));
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

    m_result->clear();
    setBusy(true);
    m_progress->start(tr("Criptografando…"));

    auto *watcher = new QFutureWatcher<PublicKeyEncryption::Result>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, fileMode, filePath]() {
        watcher->deleteLater();
        setBusy(false);

        const PublicKeyEncryption::Result result = watcher->result();
        if (!result.isSuccess()) {
            Popup::error(this, result.error);
            return;
        }

        const QString outputName = fileMode
                                       ? KeyTools::encryptedFileName(filePath)
                                       : QStringLiteral("texto_%1.enc").arg(KeyTools::timestamp());

        m_result->showResult(tr("<b>%1</b> + AES-256-GCM — %2")
                                 .arg(m_keyDescription,
                                      QLocale().formattedDataSize(result.data.size())),
                             result.data, outputName, !fileMode);
    });

    const QByteArray publicPem = m_publicPem;
    const QByteArray textBytes = text.toUtf8();
    watcher->setFuture(QtConcurrent::run([publicPem, fileMode, filePath, textBytes]() {
        PublicKeyEncryption::Result result;

        ContentPackage::Content content;
        if (fileMode) {
            const std::optional<QByteArray> data = KeyTools::readWholeFile(filePath, &result.error);
            if (!data)
                return result;
            content.fileName = QFileInfo(filePath).fileName();
            content.data = *data;
        } else {
            content.data = textBytes;
        }

        QByteArray plain = ContentPackage::pack(content);
        OpenSsl::cleanse(content.data);
        result = PublicKeyEncryption::encrypt(publicPem, plain);
        OpenSsl::cleanse(plain);
        return result;
    }));
}

void AsymmetricEncryptionScreen::setBusy(bool busy)
{
    m_encrypt->setEnabled(!busy);
    m_keyFile->setEnabled(!busy);
    m_source->setEnabled(!busy);
    if (!busy)
        m_progress->stop();
}
