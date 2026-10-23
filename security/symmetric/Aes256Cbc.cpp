#include "Aes256Cbc.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <memory>

#include "security/OpenSsl.h"
#include "security/Random.h"

namespace Aes256Cbc {

namespace {

using CipherCtx = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

std::optional<QByteArray> run(bool encrypting, const QByteArray &input, const QByteArray &key,
                              const QByteArray &iv)
{
    if (key.size() != KeySize || iv.size() != IvSize)
        return std::nullopt;

    CipherCtx ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx
        || EVP_CipherInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr, OpenSsl::bytes(key),
                             OpenSsl::bytes(iv), encrypting ? 1 : 0) != 1) {
        return std::nullopt;
    }

    QByteArray output(int(input.size()) + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    int updateLength = 0;
    int finalLength = 0;

    if (EVP_CipherUpdate(ctx.get(), OpenSsl::bytes(output), &updateLength, OpenSsl::bytes(input),
                         int(input.size())) != 1
        || EVP_CipherFinal_ex(ctx.get(), OpenSsl::bytes(output) + updateLength, &finalLength) != 1) {
        OpenSsl::lastError(); // limpa a fila de erros
        return std::nullopt;
    }

    output.resize(updateLength + finalLength);
    return output;
}

QByteArray mac(const QByteArray &data, const QByteArray &macKey)
{
    return QMessageAuthenticationCode::hash(data, macKey, QCryptographicHash::Sha256);
}

} // namespace

QByteArray encrypt(const QByteArray &plain, const QByteArray &key, const QByteArray &iv)
{
    return run(true, plain, key, iv).value_or(QByteArray());
}

std::optional<QByteArray> decrypt(const QByteArray &cipher, const QByteArray &key,
                                  const QByteArray &iv)
{
    return run(false, cipher, key, iv);
}

QByteArray seal(const QByteArray &plain, const QByteArray &sealKey)
{
    if (sealKey.size() != SealKeySize)
        return QByteArray();

    const QByteArray iv = Random::bytes(IvSize);
    if (iv.isEmpty())
        return QByteArray();

    const QByteArray cipher = encrypt(plain, sealKey.left(KeySize), iv);
    if (cipher.isEmpty())
        return QByteArray();

    const QByteArray body = iv + cipher;
    return body + mac(body, sealKey.mid(KeySize));
}

std::optional<QByteArray> open(const QByteArray &sealed, const QByteArray &sealKey)
{
    if (sealKey.size() != SealKeySize || sealed.size() < IvSize + 16 + MacSize)
        return std::nullopt;

    const QByteArray body = sealed.left(sealed.size() - MacSize);
    const QByteArray receivedMac = sealed.right(MacSize);
    const QByteArray expectedMac = mac(body, sealKey.mid(KeySize));

    if (CRYPTO_memcmp(receivedMac.constData(), expectedMac.constData(), MacSize) != 0)
        return std::nullopt;

    return decrypt(body.mid(IvSize), sealKey.left(KeySize), body.left(IvSize));
}

QByteArray deriveKey(const QByteArray &password, const QByteArray &salt, int iterations,
                     int length)
{
    QByteArray key(length, Qt::Uninitialized);
    if (PKCS5_PBKDF2_HMAC(password.constData(), int(password.size()), OpenSsl::bytes(salt),
                          int(salt.size()), iterations, EVP_sha256(), length,
                          OpenSsl::bytes(key)) != 1) {
        qWarning("Aes256Cbc::deriveKey: %s", qPrintable(OpenSsl::lastError()));
        return QByteArray();
    }
    return key;
}

} // namespace Aes256Cbc
