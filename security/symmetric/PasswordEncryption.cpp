#include "PasswordEncryption.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QObject>
#include <QtEndian>

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <memory>

#include "security/OpenSsl.h"
#include "security/Random.h"

namespace PasswordEncryption {

namespace {

const QByteArray Magic = QByteArrayLiteral("MQAENC");
constexpr quint8 Version = 1;
constexpr int HeaderSize = 6 + 1 + 1 + 4 + Aes256Cbc::SaltSize;
constexpr int HmacSize = 32;
constexpr int AeadTagSize = 16;

using CipherCtx = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

struct Spec
{
    const EVP_CIPHER *cipher;
    int keySize;
    int ivSize;
    bool aead;
};

std::optional<Spec> specFor(Algorithm algorithm)
{
    switch (algorithm) {
    case Algorithm::Aes256Cbc:        return Spec{EVP_aes_256_cbc(), 32, 16, false};
    case Algorithm::Aes192Cbc:        return Spec{EVP_aes_192_cbc(), 24, 16, false};
    case Algorithm::Aes128Cbc:        return Spec{EVP_aes_128_cbc(), 16, 16, false};
    case Algorithm::Aes256Gcm:        return Spec{EVP_aes_256_gcm(), 32, 12, true};
    case Algorithm::ChaCha20Poly1305: return Spec{EVP_chacha20_poly1305(), 32, 12, true};
    }
    return std::nullopt;
}

QByteArray hmac(const QByteArray &data, const QByteArray &key)
{
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

// Executa a cifra. Em AEAD, `aad` é autenticado e `tag` é gerada (encrypt) ou conferida (decrypt).
std::optional<QByteArray> runCipher(const Spec &spec, bool encrypting, const QByteArray &input,
                                    const QByteArray &key, const QByteArray &iv,
                                    const QByteArray &aad, QByteArray *tag)
{
    CipherCtx ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx || EVP_CipherInit_ex(ctx.get(), spec.cipher, nullptr, nullptr, nullptr, encrypting) != 1)
        return std::nullopt;

    if (spec.aead
        && EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, spec.ivSize, nullptr) != 1)
        return std::nullopt;

    if (EVP_CipherInit_ex(ctx.get(), nullptr, nullptr, OpenSsl::bytes(key), OpenSsl::bytes(iv),
                          encrypting)
        != 1)
        return std::nullopt;

    int length = 0;
    if (spec.aead && !aad.isEmpty()
        && EVP_CipherUpdate(ctx.get(), nullptr, &length, OpenSsl::bytes(aad), int(aad.size())) != 1)
        return std::nullopt;

    QByteArray output(int(input.size()) + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    int updateLength = 0;
    if (EVP_CipherUpdate(ctx.get(), OpenSsl::bytes(output), &updateLength, OpenSsl::bytes(input),
                         int(input.size()))
        != 1)
        return std::nullopt;

    if (spec.aead && !encrypting
        && EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_TAG, AeadTagSize, tag->data()) != 1)
        return std::nullopt;

    int finalLength = 0;
    if (EVP_CipherFinal_ex(ctx.get(), OpenSsl::bytes(output) + updateLength, &finalLength) != 1) {
        OpenSsl::lastError();
        return std::nullopt;
    }

    if (spec.aead && encrypting) {
        tag->resize(AeadTagSize);
        if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_GET_TAG, AeadTagSize, tag->data()) != 1)
            return std::nullopt;
    }

    output.resize(updateLength + finalLength);
    return output;
}

} // namespace

QList<AlgorithmInfo> algorithms()
{
    return {
        {Algorithm::Aes256Cbc, QStringLiteral("AES-256-CBC"),
         QObject::tr("Padrão do aplicativo (com HMAC-SHA256)")},
        {Algorithm::Aes256Gcm, QStringLiteral("AES-256-GCM"),
         QObject::tr("Autenticado (AEAD), rápido com aceleração AES")},
        {Algorithm::ChaCha20Poly1305, QStringLiteral("ChaCha20-Poly1305"),
         QObject::tr("Autenticado (AEAD), rápido sem aceleração AES")},
        {Algorithm::Aes192Cbc, QStringLiteral("AES-192-CBC"), QObject::tr("Com HMAC-SHA256")},
        {Algorithm::Aes128Cbc, QStringLiteral("AES-128-CBC"), QObject::tr("Com HMAC-SHA256")},
    };
}

QString name(Algorithm algorithm)
{
    for (const AlgorithmInfo &info : algorithms()) {
        if (info.algorithm == algorithm)
            return info.name;
    }
    return QString();
}

QByteArray encrypt(Algorithm algorithm, const QByteArray &plain, const QByteArray &password,
                   int iterations)
{
    const std::optional<Spec> spec = specFor(algorithm);
    if (!spec || password.isEmpty())
        return QByteArray();

    const QByteArray salt = Random::bytes(Aes256Cbc::SaltSize);
    const QByteArray iv = Random::bytes(spec->ivSize);
    if (salt.isEmpty() || iv.isEmpty())
        return QByteArray();

    // CBC: chave do cifrador + 32 bytes para o HMAC
    const int macKeySize = spec->aead ? 0 : HmacSize;
    QByteArray keys = Aes256Cbc::deriveKey(password, salt, iterations, spec->keySize + macKeySize);
    if (keys.isEmpty())
        return QByteArray();

    QByteArray header = Magic;
    header.append(char(Version));
    header.append(char(algorithm));
    char iterationBytes[4];
    qToBigEndian(quint32(iterations), iterationBytes);
    header.append(iterationBytes, 4);
    header.append(salt);

    QByteArray tag;
    const std::optional<QByteArray> cipher =
        runCipher(*spec, true, plain, keys.left(spec->keySize), iv, header + iv, &tag);

    QByteArray result;
    if (cipher) {
        result = header + iv + *cipher;
        result += spec->aead ? tag : hmac(result, keys.mid(spec->keySize));
    }

    OpenSsl::cleanse(keys);
    return result;
}

std::optional<QByteArray> decrypt(const QByteArray &encrypted, const QByteArray &password)
{
    if (encrypted.size() < HeaderSize || !encrypted.startsWith(Magic)
        || quint8(encrypted.at(6)) != Version)
        return std::nullopt;

    const std::optional<Spec> spec = specFor(Algorithm(quint8(encrypted.at(7))));
    if (!spec)
        return std::nullopt;

    const int tagSize = spec->aead ? AeadTagSize : HmacSize;
    if (encrypted.size() < HeaderSize + spec->ivSize + tagSize)
        return std::nullopt;

    const int iterations = int(qFromBigEndian<quint32>(encrypted.constData() + 8));
    const QByteArray header = encrypted.left(HeaderSize);
    const QByteArray salt = header.right(Aes256Cbc::SaltSize);
    const QByteArray iv = encrypted.mid(HeaderSize, spec->ivSize);
    const QByteArray body = encrypted.left(encrypted.size() - tagSize);
    QByteArray tag = encrypted.right(tagSize);
    const QByteArray cipher = body.mid(HeaderSize + spec->ivSize);

    const int macKeySize = spec->aead ? 0 : HmacSize;
    QByteArray keys = Aes256Cbc::deriveKey(password, salt, iterations, spec->keySize + macKeySize);
    if (keys.isEmpty())
        return std::nullopt;

    std::optional<QByteArray> plain;
    if (spec->aead) {
        plain = runCipher(*spec, false, cipher, keys.left(spec->keySize), iv, header + iv, &tag);
    } else {
        const QByteArray expected = hmac(body, keys.mid(spec->keySize));
        if (CRYPTO_memcmp(expected.constData(), tag.constData(), HmacSize) == 0)
            plain = runCipher(*spec, false, cipher, keys.left(spec->keySize), iv, QByteArray(), &tag);
    }

    OpenSsl::cleanse(keys);
    return plain;
}

} // namespace PasswordEncryption
