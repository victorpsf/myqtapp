#include "SymmetricDecryptionScreen.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QPushButton>
#include <QtConcurrent/QtConcurrentRun>

#include "components/fields/ContentSourceField.h"
#include "components/fields/InputField.h"
#include "components/forms/SectionCard.h"
#include "components/labels/ProgressStatus.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "screens/keys/DecryptedResultCard.h"
#include "screens/keys/KeyToolsCommon.h"
#include "security/ContentPackage.h"
#include "security/OpenSsl.h"
#include "security/symmetric/PasswordEncryption.h"

namespace {

struct DecryptionResult
{
    ContentPackage::Content content;
    QString algorithm;
    QString error;
};

// O algoritmo fica gravado no cabeçalho "MQAENC" (byte 7)
QString algorithmName(const QByteArray &encrypted)
{
    if (encrypted.size() < 8 || !encrypted.startsWith("MQAENC"))
        return QString();
    return PasswordEncryption::name(PasswordEncryption::Algorithm(quint8(encrypted.at(7))));
}

} // namespace

SymmetricDecryptionScreen::SymmetricDecryptionScreen(QWidget *parent)
    : ContentPage(tr("Descriptografia simétrica"),
                  tr("Descriptografa um texto ou arquivo gerado em \"Criptografar (simétrica)\". "
                     "O algoritmo é identificado automaticamente; basta informar a senha."),
                  parent)
    , m_password(new InputField(tr("Senha")))
    , m_source(new ContentSourceField(tr("Descriptografar:"),
                                      tr("Conteúdo criptografado (base64)"),
                                      tr("Arquivo criptografado")))
    , m_decrypt(new QPushButton(tr("Descriptografar")))
    , m_progress(new ProgressStatus())
    , m_result(new DecryptedResultCard())
{
    m_password->setPasswordMode(true);
    m_source->setPlaceholder(tr("Cole aqui o base64 gerado na criptografia"));
    m_source->setMonospace(true);

    m_decrypt->setProperty("primary", true);
    m_decrypt->setCursor(Qt::PointingHandCursor);

    auto *settings = new SectionCard(tr("Configuração"));
    settings->addWidget(m_password);
    settings->addWidget(m_source);

    auto *actions = new QHBoxLayout();
    actions->addWidget(m_progress, 1);
    actions->addStretch();
    actions->addWidget(m_decrypt);
    settings->addLayout(actions);

    addContent(settings);
    addContent(m_result);

    connect(m_source, &ContentSourceField::changed, m_result, &DecryptedResultCard::clear);
    connect(m_password, &InputField::submitted, this, &SymmetricDecryptionScreen::decrypt);
    connect(m_decrypt, &QPushButton::clicked, this, &SymmetricDecryptionScreen::decrypt);
}

void SymmetricDecryptionScreen::decrypt()
{
    if (!m_decrypt->isEnabled())
        return;

    const QString password = m_password->text();
    if (password.isEmpty()) {
        Popup::warning(this, tr("Informe a senha."));
        m_password->focusInput();
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
    m_progress->start(tr("Derivando a chave da senha e descriptografando…"));

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
        m_result->showContent(tr("<b>%1</b> — descriptografado com sucesso").arg(result.algorithm),
                              result.content, fallbackName);
    });

    QByteArray passwordBytes = password.toUtf8();
    watcher->setFuture(QtConcurrent::run(
        [passwordBytes, fileMode, filePath, encrypted]() mutable {
            DecryptionResult result;

            if (fileMode) {
                const std::optional<QByteArray> data =
                    KeyTools::readWholeFile(filePath, &result.error);
                if (!data) {
                    OpenSsl::cleanse(passwordBytes);
                    return result;
                }
                encrypted = *data;
            }

            result.algorithm = algorithmName(encrypted);
            if (result.algorithm.isEmpty()) {
                OpenSsl::cleanse(passwordBytes);
                result.error = QObject::tr("O conteúdo não foi gerado por \"Criptografar "
                                           "(simétrica)\" ou está corrompido.");
                return result;
            }

            std::optional<QByteArray> plain = PasswordEncryption::decrypt(encrypted, passwordBytes);
            OpenSsl::cleanse(passwordBytes);
            if (!plain) {
                result.error = QObject::tr("Senha incorreta ou conteúdo alterado.");
                return result;
            }

            result.content = ContentPackage::unpack(*plain);
            OpenSsl::cleanse(*plain);
            return result;
        }));
    OpenSsl::cleanse(passwordBytes);
}

void SymmetricDecryptionScreen::setBusy(bool busy)
{
    m_decrypt->setEnabled(!busy);
    m_password->setEnabled(!busy);
    m_source->setEnabled(!busy);
    if (!busy)
        m_progress->stop();
}
