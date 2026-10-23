#pragma once

#include <QByteArray>

#include <atomic>
#include <memory>
#include <optional>

struct evp_pkey_st; // EVP_PKEY do OpenSSL

// Chave RSA (par completo ou só a pública).
// Criptografia: RSA-OAEP com SHA-256 | Assinatura: RSA-PSS com SHA-256
class RsaKey
{
public:
    static constexpr int DefaultBits = 8192;

    RsaKey() = default;

    // Bloqueante: com 8192 bits leva de segundos a minutos. Rode fora da thread de UI.
    // Se `cancel` virar true durante a geração, ela é abortada e retorna uma chave nula.
    static RsaKey generate(int bits = DefaultBits, const std::atomic_bool *cancel = nullptr);

    static RsaKey fromPublicPem(const QByteArray &pem);
    static RsaKey fromPrivatePem(const QByteArray &pem, const QByteArray &passphrase = QByteArray());

    bool isNull() const { return !m_key; }
    bool hasPrivateKey() const { return m_hasPrivateKey; }
    int bits() const;

    // Tamanho máximo que cabe em um encrypt (OAEP-SHA256): bytes da chave - 66
    int maxEncryptSize() const;

    QByteArray publicPem() const;

    // PKCS#8; com passphrase a chave sai protegida com AES-256-CBC
    QByteArray privatePem(const QByteArray &passphrase = QByteArray()) const;

    QByteArray encrypt(const QByteArray &plain) const;
    std::optional<QByteArray> decrypt(const QByteArray &cipher) const;

    QByteArray sign(const QByteArray &data) const;
    bool verify(const QByteArray &data, const QByteArray &signature) const;

private:
    RsaKey(std::shared_ptr<evp_pkey_st> key, bool hasPrivateKey);

    std::shared_ptr<evp_pkey_st> m_key;
    bool m_hasPrivateKey = false;
};
