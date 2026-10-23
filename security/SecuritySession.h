#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>

#include <atomic>
#include <memory>
#include <optional>

#include "models/responses/EncryptedPayload.h"
#include "security/asymmetric/Rsa.h"

// Material criptográfico da sessão do usuário logado.
//
// - Ao logar, begin() gera (em segundo plano) o par RSA do cliente e deriva,
//   a partir da senha, a chave local usada no AES-256-CBC dos arquivos.
// - A chave privada e a chave local ficam SOMENTE em memória e são apagadas em clear().
// - As chaves públicas (cliente e servidores) são gravadas pelo KeyStore.
class SecuritySession : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Generating, Ready, Failed };

    static SecuritySession &instance();

    void begin(const QString &login, const QString &password);
    void clear();

    State state() const { return m_state; }
    QString errorString() const { return m_error; }

    const RsaKey &clientKey() const { return m_clientKey; }

    // Chave privada exportada em PEM protegido (AES-256-CBC) com a senha informada
    QByteArray exportPrivateKey(const QString &password) const;

    // Handshake: chave pública recebida de cada microserviço
    bool hasServerKey(const QString &service) const;
    bool setServerKey(const QString &service, const QByteArray &pem);

    // Mensagens críticas cliente <-> servidor
    std::optional<EncryptedPayload> sealFor(const QString &service, const QByteArray &plain) const;
    std::optional<QByteArray> openFrom(const QString &service, const EncryptedPayload &payload) const;

    // Armazenamento local com AES-256-CBC (chave derivada da senha)
    QByteArray encryptLocal(const QByteArray &plain) const;
    std::optional<QByteArray> decryptLocal(const QByteArray &sealed) const;
    bool saveEncryptedFile(const QString &fileName, const QByteArray &plain) const;
    std::optional<QByteArray> loadEncryptedFile(const QString &fileName) const;

signals:
    void keysReady();
    void keysFailed(const QString &error);

private:
    explicit SecuritySession(QObject *parent = nullptr);

    State m_state = State::Idle;
    QString m_error;
    quint64 m_generation = 0;
    std::shared_ptr<std::atomic_bool> m_cancelGeneration;

    RsaKey m_clientKey;
    QByteArray m_localKey;
    QHash<QString, RsaKey> m_serverKeys;
};
