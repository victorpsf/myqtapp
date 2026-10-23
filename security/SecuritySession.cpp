#include "SecuritySession.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

#include "security/KeyStore.h"
#include "security/OpenSsl.h"
#include "security/symmetric/Aes256Cbc.h"

namespace {

struct KeyMaterial
{
    RsaKey clientKey;
    QByteArray localKey;
    QString error;
};

} // namespace

SecuritySession &SecuritySession::instance()
{
    static SecuritySession session;
    return session;
}

SecuritySession::SecuritySession(QObject *parent)
    : QObject(parent)
{
}

void SecuritySession::begin(const QString &login, const QString &password)
{
    clear();

    const quint64 generation = m_generation;
    m_state = State::Generating;

    auto cancel = std::make_shared<std::atomic_bool>(false);
    m_cancelGeneration = cancel;

    const QByteArray salt = KeyStore::loadOrCreateSalt(login);
    QByteArray passwordBytes = password.toUtf8();

    auto *watcher = new QFutureWatcher<KeyMaterial>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, generation]() {
        watcher->deleteLater();

        // Logout durante a geração: descarta o resultado
        if (generation != m_generation)
            return;

        KeyMaterial result = watcher->result();
        if (!result.error.isEmpty()) {
            m_state = State::Failed;
            m_error = result.error;
            emit keysFailed(m_error);
            return;
        }

        m_clientKey = result.clientKey;
        m_localKey = result.localKey;
        OpenSsl::cleanse(result.localKey);

        KeyStore::saveClientPublicKey(m_clientKey.publicPem());

        m_state = State::Ready;
        emit keysReady();
    });

    watcher->setFuture(QtConcurrent::run([passwordBytes, salt, cancel]() mutable {
        KeyMaterial result;

        result.localKey = Aes256Cbc::deriveKey(passwordBytes, salt);
        OpenSsl::cleanse(passwordBytes);

        result.clientKey = RsaKey::generate(RsaKey::DefaultBits, cancel.get());

        if (cancel->load())
            result.error = QObject::tr("Geração da chave cancelada.");
        else if (result.localKey.isEmpty())
            result.error = QObject::tr("Não foi possível derivar a chave local.");
        else if (result.clientKey.isNull())
            result.error = QObject::tr("Não foi possível gerar a chave RSA.");

        return result;
    }));

    OpenSsl::cleanse(passwordBytes);
}

void SecuritySession::clear()
{
    if (m_cancelGeneration) {
        m_cancelGeneration->store(true); // aborta uma geração RSA em andamento
        m_cancelGeneration.reset();
    }

    ++m_generation;
    m_state = State::Idle;
    m_error.clear();
    m_clientKey = RsaKey();
    OpenSsl::cleanse(m_localKey);
    m_serverKeys.clear();
}

QByteArray SecuritySession::exportPrivateKey(const QString &password) const
{
    if (m_state != State::Ready || password.isEmpty())
        return QByteArray();

    QByteArray passphrase = password.toUtf8();
    const QByteArray pem = m_clientKey.privatePem(passphrase);
    OpenSsl::cleanse(passphrase);
    return pem;
}

bool SecuritySession::hasServerKey(const QString &service) const
{
    return m_serverKeys.contains(service);
}

bool SecuritySession::setServerKey(const QString &service, const QByteArray &pem)
{
    const RsaKey key = RsaKey::fromPublicPem(pem);
    if (key.isNull())
        return false;

    m_serverKeys.insert(service, key);
    KeyStore::saveServerPublicKey(service, pem);
    return true;
}

std::optional<EncryptedPayload> SecuritySession::sealFor(const QString &service,
                                                         const QByteArray &plain) const
{
    if (m_state != State::Ready || !hasServerKey(service))
        return std::nullopt;

    const std::optional<SealedMessage> sealed =
        SecureEnvelope::seal(plain, m_serverKeys.value(service), m_clientKey);
    if (!sealed)
        return std::nullopt;

    return EncryptedPayload::fromSealed(*sealed);
}

std::optional<QByteArray> SecuritySession::openFrom(const QString &service,
                                                    const EncryptedPayload &payload) const
{
    if (m_state != State::Ready || !hasServerKey(service))
        return std::nullopt;

    return SecureEnvelope::open(payload.toSealed(), m_clientKey, m_serverKeys.value(service));
}

QByteArray SecuritySession::encryptLocal(const QByteArray &plain) const
{
    if (m_state != State::Ready)
        return QByteArray();
    return Aes256Cbc::seal(plain, m_localKey);
}

std::optional<QByteArray> SecuritySession::decryptLocal(const QByteArray &sealed) const
{
    if (m_state != State::Ready)
        return std::nullopt;
    return Aes256Cbc::open(sealed, m_localKey);
}

bool SecuritySession::saveEncryptedFile(const QString &fileName, const QByteArray &plain) const
{
    const QByteArray sealed = encryptLocal(plain);
    if (sealed.isEmpty())
        return false;
    return KeyStore::writeFile(QDir(KeyStore::dataDirectory()).filePath(QFileInfo(fileName).fileName()),
                               sealed);
}

std::optional<QByteArray> SecuritySession::loadEncryptedFile(const QString &fileName) const
{
    const QByteArray sealed =
        KeyStore::readFile(QDir(KeyStore::dataDirectory()).filePath(QFileInfo(fileName).fileName()));
    if (sealed.isEmpty())
        return std::nullopt;
    return decryptLocal(sealed);
}
