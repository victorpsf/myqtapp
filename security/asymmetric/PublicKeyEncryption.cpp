#include "PublicKeyEncryption.h"

#include <QObject>
#include <QtEndian>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <cstring>
#include <memory>
#include <optional>

#include "security/OpenSsl.h"
#include "security/Random.h"

namespace PublicKeyEncryption {

namespace {

const QByteArray Magic = QByteArrayLiteral("MQAPKE");
const QByteArray HkdfInfo = QByteArrayLiteral("myqtapp-pke-v1");
constexpr quint8 Version = 1;
constexpr int PrefixSize = 6 + 1 + 1 + 2; // magic | versão | esquema | tamanho do bloco
constexpr int ContentKeySize = 32;
constexpr int NonceSize = 12;
constexpr int TagSize = 16;

// OAEP-SHA256 consome 2 * 32 + 2 bytes do módulo
constexpr int RsaOaepOverhead = 66;

enum class Scheme : quint8 {
    RsaOaep = 1,
    Ecdh = 2,
};

using PkeyCtx = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
using Pkey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using Bio = std::unique_ptr<BIO, decltype(&BIO_free)>;
using CipherCtx = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

Result failure(const QString &message)
{
    OpenSsl::lastError(); // descarta a fila de erros
    Result result;
    result.error = message;
    return result;
}

// Fornece a senha ao OpenSSL. Sem senha devolve 0, evitando que o OpenSSL
// pergunte a senha no terminal.
int passwordCallback(char *buffer, int size, int, void *userData)
{
    const auto *passphrase = static_cast<const QByteArray *>(userData);
    if (!passphrase || passphrase->isEmpty())
        return 0;
    const int length = qMin(size, int(passphrase->size()));
    std::memcpy(buffer, passphrase->constData(), size_t(length));
    return length;
}

bool isEncryptedPem(const QByteArray &pem)
{
    return pem.contains("ENCRYPTED PRIVATE KEY") || pem.contains("Proc-Type: 4,ENCRYPTED");
}

Pkey readPublic(const QByteArray &pem)
{
    Bio bio(BIO_new_mem_buf(pem.constData(), int(pem.size())), BIO_free);
    EVP_PKEY *key = bio ? PEM_read_bio_PUBKEY(bio.get(), nullptr, passwordCallback, nullptr)
                        : nullptr;
    return Pkey(key, EVP_PKEY_free);
}

Pkey readPrivate(const QByteArray &pem, const QByteArray &passphrase)
{
    Bio bio(BIO_new_mem_buf(pem.constData(), int(pem.size())), BIO_free);
    EVP_PKEY *key = bio ? PEM_read_bio_PrivateKey(bio.get(), nullptr, passwordCallback,
                                                  const_cast<QByteArray *>(&passphrase))
                        : nullptr;
    return Pkey(key, EVP_PKEY_free);
}

bool isEcdhType(int type)
{
    return type == EVP_PKEY_EC || type == EVP_PKEY_X25519 || type == EVP_PKEY_X448;
}

QString describe(EVP_PKEY *key)
{
    const int bits = EVP_PKEY_bits(key);
    switch (EVP_PKEY_base_id(key)) {
    case EVP_PKEY_RSA:     return QStringLiteral("RSA %1 bits").arg(bits);
    case EVP_PKEY_EC:      return QObject::tr("Curva elíptica (EC) %1 bits").arg(bits);
    case EVP_PKEY_X25519:  return QStringLiteral("X25519");
    case EVP_PKEY_X448:    return QStringLiteral("X448");
    case EVP_PKEY_ED25519: return QStringLiteral("Ed25519");
    case EVP_PKEY_ED448:   return QStringLiteral("Ed448");
    }
    return QObject::tr("Tipo desconhecido");
}

// Motivo para não aceitar a chave, ou vazio se ela pode criptografar
QString unsupportedReason(EVP_PKEY *key)
{
    const int type = EVP_PKEY_base_id(key);
    if (type == EVP_PKEY_RSA) {
        if (EVP_PKEY_size(key) - RsaOaepOverhead < ContentKeySize)
            return QObject::tr("A chave RSA é pequena demais para criptografar (mínimo 1024 bits).");
        return QString();
    }
    if (isEcdhType(type))
        return QString();
    if (type == EVP_PKEY_ED25519 || type == EVP_PKEY_ED448)
        return QObject::tr("Chaves Ed25519/Ed448 servem apenas para assinatura. "
                           "Use uma chave RSA ou de curva elíptica (ECDSA).");
    return QObject::tr("Tipo de chave não suportado para criptografia.");
}

QByteArray publicDer(EVP_PKEY *key)
{
    const int length = i2d_PUBKEY(key, nullptr);
    if (length <= 0)
        return QByteArray();
    QByteArray der(length, Qt::Uninitialized);
    unsigned char *out = OpenSsl::bytes(der);
    if (i2d_PUBKEY(key, &out) != length)
        return QByteArray();
    return der;
}

Pkey fromPublicDer(const QByteArray &der)
{
    const unsigned char *in = OpenSsl::bytes(der);
    return Pkey(d2i_PUBKEY(nullptr, &in, long(der.size())), EVP_PKEY_free);
}

bool setOaep(EVP_PKEY_CTX *ctx)
{
    return EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) > 0
           && EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) > 0
           && EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) > 0;
}

std::optional<QByteArray> rsaWrap(EVP_PKEY *key, const QByteArray &contentKey)
{
    PkeyCtx ctx(EVP_PKEY_CTX_new(key, nullptr), EVP_PKEY_CTX_free);
    size_t length = 0;
    if (!ctx || EVP_PKEY_encrypt_init(ctx.get()) <= 0 || !setOaep(ctx.get())
        || EVP_PKEY_encrypt(ctx.get(), nullptr, &length, OpenSsl::bytes(contentKey),
                            size_t(contentKey.size()))
               <= 0)
        return std::nullopt;

    QByteArray wrapped(int(length), Qt::Uninitialized);
    if (EVP_PKEY_encrypt(ctx.get(), OpenSsl::bytes(wrapped), &length, OpenSsl::bytes(contentKey),
                         size_t(contentKey.size()))
        <= 0)
        return std::nullopt;
    wrapped.resize(int(length));
    return wrapped;
}

std::optional<QByteArray> rsaUnwrap(EVP_PKEY *key, const QByteArray &wrapped)
{
    PkeyCtx ctx(EVP_PKEY_CTX_new(key, nullptr), EVP_PKEY_CTX_free);
    size_t length = 0;
    if (!ctx || EVP_PKEY_decrypt_init(ctx.get()) <= 0 || !setOaep(ctx.get())
        || EVP_PKEY_decrypt(ctx.get(), nullptr, &length, OpenSsl::bytes(wrapped),
                            size_t(wrapped.size()))
               <= 0)
        return std::nullopt;

    QByteArray contentKey(int(length), Qt::Uninitialized);
    if (EVP_PKEY_decrypt(ctx.get(), OpenSsl::bytes(contentKey), &length, OpenSsl::bytes(wrapped),
                         size_t(wrapped.size()))
        <= 0) {
        OpenSsl::cleanse(contentKey);
        return std::nullopt;
    }
    contentKey.resize(int(length));
    return contentKey;
}

// Gera uma chave efêmera com os mesmos parâmetros (curva) da chave do destinatário
Pkey generateEphemeral(EVP_PKEY *peer)
{
    PkeyCtx ctx(EVP_PKEY_CTX_new(peer, nullptr), EVP_PKEY_CTX_free);
    EVP_PKEY *key = nullptr;
    if (!ctx || EVP_PKEY_keygen_init(ctx.get()) <= 0 || EVP_PKEY_keygen(ctx.get(), &key) <= 0)
        return Pkey(nullptr, EVP_PKEY_free);
    return Pkey(key, EVP_PKEY_free);
}

std::optional<QByteArray> ecdh(EVP_PKEY *own, EVP_PKEY *peer)
{
    PkeyCtx ctx(EVP_PKEY_CTX_new(own, nullptr), EVP_PKEY_CTX_free);
    size_t length = 0;
    if (!ctx || EVP_PKEY_derive_init(ctx.get()) <= 0
        || EVP_PKEY_derive_set_peer(ctx.get(), peer) <= 0
        || EVP_PKEY_derive(ctx.get(), nullptr, &length) <= 0)
        return std::nullopt;

    QByteArray secret(int(length), Qt::Uninitialized);
    if (EVP_PKEY_derive(ctx.get(), OpenSsl::bytes(secret), &length) <= 0) {
        OpenSsl::cleanse(secret);
        return std::nullopt;
    }
    secret.resize(int(length));
    return secret;
}

// HKDF-SHA256(segredo, salt = pública efêmera, info = rótulo | pública do destinatário)
std::optional<QByteArray> hkdf(const QByteArray &secret, const QByteArray &salt,
                               const QByteArray &info)
{
    PkeyCtx ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr), EVP_PKEY_CTX_free);
    if (!ctx || EVP_PKEY_derive_init(ctx.get()) <= 0
        || EVP_PKEY_CTX_set_hkdf_md(ctx.get(), EVP_sha256()) <= 0
        || EVP_PKEY_CTX_set1_hkdf_salt(ctx.get(), OpenSsl::bytes(salt), int(salt.size())) <= 0
        || EVP_PKEY_CTX_set1_hkdf_key(ctx.get(), OpenSsl::bytes(secret), int(secret.size())) <= 0
        || EVP_PKEY_CTX_add1_hkdf_info(ctx.get(), OpenSsl::bytes(info), int(info.size())) <= 0)
        return std::nullopt;

    QByteArray key(ContentKeySize, Qt::Uninitialized);
    size_t length = size_t(key.size());
    if (EVP_PKEY_derive(ctx.get(), OpenSsl::bytes(key), &length) <= 0
        || length != size_t(ContentKeySize)) {
        OpenSsl::cleanse(key);
        return std::nullopt;
    }
    return key;
}

std::optional<QByteArray> ecdhContentKey(EVP_PKEY *own, EVP_PKEY *peer,
                                         const QByteArray &ephemeralDer,
                                         const QByteArray &recipientDer)
{
    std::optional<QByteArray> secret = ecdh(own, peer);
    if (!secret)
        return std::nullopt;
    std::optional<QByteArray> key = hkdf(*secret, ephemeralDer, HkdfInfo + recipientDer);
    OpenSsl::cleanse(*secret);
    return key;
}

// AES-256-GCM. Ao criptografar `tag` é preenchida; ao descriptografar ela é conferida.
std::optional<QByteArray> gcm(bool encrypting, const QByteArray &input, const QByteArray &key,
                              const QByteArray &nonce, const QByteArray &aad, QByteArray &tag)
{
    CipherCtx ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx
        || EVP_CipherInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr, encrypting)
               != 1
        || EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, NonceSize, nullptr) != 1
        || EVP_CipherInit_ex(ctx.get(), nullptr, nullptr, OpenSsl::bytes(key),
                             OpenSsl::bytes(nonce), encrypting)
               != 1)
        return std::nullopt;

    int length = 0;
    if (EVP_CipherUpdate(ctx.get(), nullptr, &length, OpenSsl::bytes(aad), int(aad.size())) != 1)
        return std::nullopt;

    QByteArray output(int(input.size()) + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    int updateLength = 0;
    if (EVP_CipherUpdate(ctx.get(), OpenSsl::bytes(output), &updateLength, OpenSsl::bytes(input),
                         int(input.size()))
        != 1)
        return std::nullopt;

    if (!encrypting
        && EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_TAG, TagSize, tag.data()) != 1)
        return std::nullopt;

    int finalLength = 0;
    if (EVP_CipherFinal_ex(ctx.get(), OpenSsl::bytes(output) + updateLength, &finalLength) != 1) {
        OpenSsl::cleanse(output);
        return std::nullopt;
    }

    if (encrypting) {
        tag.resize(TagSize);
        if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_GET_TAG, TagSize, tag.data()) != 1)
            return std::nullopt;
    }

    output.resize(updateLength + finalLength);
    return output;
}

} // namespace

bool looksEncrypted(const QByteArray &data)
{
    return data.size() >= PrefixSize + NonceSize + TagSize && data.startsWith(Magic);
}

KeyInfo inspect(const QByteArray &pem)
{
    KeyInfo info;

    if (pem.contains("-----BEGIN PUBLIC KEY-----")) {
        Pkey key = readPublic(pem);
        if (!key) {
            OpenSsl::lastError();
            info.error = QObject::tr("Chave pública PEM inválida.");
            return info;
        }
        info.description = describe(key.get());
        info.canEncrypt = unsupportedReason(key.get()).isEmpty();
        return info;
    }

    if (!pem.contains("PRIVATE KEY-----")) {
        info.error = QObject::tr("O arquivo não contém uma chave em formato PEM.");
        return info;
    }

    info.isPrivate = true;
    if (isEncryptedPem(pem)) {
        info.isEncrypted = true;
        info.description = QObject::tr("Chave privada protegida por senha");
        return info;
    }

    Pkey key = readPrivate(pem, QByteArray());
    if (!key) {
        OpenSsl::lastError();
        info.error = QObject::tr("Chave privada PEM inválida.");
        return info;
    }
    info.description = describe(key.get());
    info.canEncrypt = unsupportedReason(key.get()).isEmpty();
    return info;
}

Result encrypt(const QByteArray &publicPem, const QByteArray &plain)
{
    Pkey recipient = readPublic(publicPem);
    if (!recipient)
        return failure(QObject::tr("O arquivo escolhido não contém uma chave pública PEM válida."));

    const QString reason = unsupportedReason(recipient.get());
    if (!reason.isEmpty())
        return failure(reason);

    Scheme scheme = Scheme::RsaOaep;
    QByteArray block;
    QByteArray contentKey;

    if (EVP_PKEY_base_id(recipient.get()) == EVP_PKEY_RSA) {
        contentKey = Random::bytes(ContentKeySize);
        const std::optional<QByteArray> wrapped =
            contentKey.isEmpty() ? std::nullopt : rsaWrap(recipient.get(), contentKey);
        if (!wrapped) {
            OpenSsl::cleanse(contentKey);
            return failure(QObject::tr("Falha ao cifrar a chave com RSA."));
        }
        block = *wrapped;
    } else {
        scheme = Scheme::Ecdh;
        Pkey ephemeral = generateEphemeral(recipient.get());
        if (!ephemeral)
            return failure(QObject::tr("Falha ao gerar a chave efêmera."));

        block = publicDer(ephemeral.get());
        const QByteArray recipientDer = publicDer(recipient.get());
        const std::optional<QByteArray> derived =
            block.isEmpty() || recipientDer.isEmpty()
                ? std::nullopt
                : ecdhContentKey(ephemeral.get(), recipient.get(), block, recipientDer);
        if (!derived)
            return failure(QObject::tr("Falha na troca de chaves (ECDH)."));
        contentKey = *derived;
    }

    const QByteArray nonce = Random::bytes(NonceSize);
    if (nonce.isEmpty() || block.size() > 0xFFFF) {
        OpenSsl::cleanse(contentKey);
        return failure(QObject::tr("Falha ao preparar a criptografia."));
    }

    QByteArray header = Magic;
    header.append(char(Version));
    header.append(char(scheme));
    char blockSize[2];
    qToBigEndian(quint16(block.size()), blockSize);
    header.append(blockSize, 2);
    header.append(block);

    QByteArray tag;
    const std::optional<QByteArray> cipher = gcm(true, plain, contentKey, nonce, header + nonce, tag);
    OpenSsl::cleanse(contentKey);
    if (!cipher)
        return failure(QObject::tr("Falha ao criptografar o conteúdo."));

    Result result;
    result.data = header + nonce + *cipher + tag;
    return result;
}

Result decrypt(const QByteArray &privatePem, const QByteArray &passphrase,
               const QByteArray &encrypted)
{
    if (!looksEncrypted(encrypted) || quint8(encrypted.at(6)) != Version)
        return failure(QObject::tr("O conteúdo não foi criptografado por esta ferramenta "
                                   "(criptografia assimétrica) ou está corrompido."));

    const auto scheme = Scheme(quint8(encrypted.at(7)));
    const int blockSize = qFromBigEndian<quint16>(encrypted.constData() + 8);
    const int headerSize = PrefixSize + blockSize;
    if (encrypted.size() < headerSize + NonceSize + TagSize)
        return failure(QObject::tr("Conteúdo criptografado incompleto ou corrompido."));

    Pkey key = readPrivate(privatePem, passphrase);
    if (!key) {
        if (passphrase.isEmpty() && isEncryptedPem(privatePem))
            return failure(QObject::tr("A chave privada é protegida por senha. Informe a senha."));
        if (!privatePem.contains("PRIVATE KEY-----"))
            return failure(QObject::tr("O arquivo escolhido não contém uma chave privada PEM."));
        return failure(QObject::tr("Senha incorreta ou chave privada inválida."));
    }

    const int keyType = EVP_PKEY_base_id(key.get());
    const QByteArray block = encrypted.mid(PrefixSize, blockSize);
    std::optional<QByteArray> contentKey;

    switch (scheme) {
    case Scheme::RsaOaep:
        if (keyType != EVP_PKEY_RSA)
            return failure(QObject::tr("Este conteúdo foi criptografado com uma chave RSA. "
                                       "Escolha a chave privada RSA correspondente."));
        contentKey = rsaUnwrap(key.get(), block);
        break;
    case Scheme::Ecdh: {
        if (!isEcdhType(keyType))
            return failure(QObject::tr("Este conteúdo foi criptografado com uma chave de curva "
                                       "elíptica. Escolha a chave privada correspondente."));
        Pkey ephemeral = fromPublicDer(block);
        const QByteArray recipientDer = publicDer(key.get());
        if (ephemeral && !recipientDer.isEmpty())
            contentKey = ecdhContentKey(key.get(), ephemeral.get(), block, recipientDer);
        break;
    }
    default:
        return failure(QObject::tr("Esquema de criptografia desconhecido."));
    }

    const QString wrongKey =
        QObject::tr("Não foi possível descriptografar: a chave privada não corresponde à chave "
                    "pública usada, ou o conteúdo foi alterado.");

    if (!contentKey || contentKey->size() != ContentKeySize) {
        if (contentKey)
            OpenSsl::cleanse(*contentKey);
        return failure(wrongKey);
    }

    const QByteArray nonce = encrypted.mid(headerSize, NonceSize);
    const QByteArray cipher =
        encrypted.mid(headerSize + NonceSize, encrypted.size() - headerSize - NonceSize - TagSize);
    QByteArray tag = encrypted.right(TagSize);

    const std::optional<QByteArray> plain =
        gcm(false, cipher, *contentKey, nonce, encrypted.left(headerSize) + nonce, tag);
    OpenSsl::cleanse(*contentKey);
    if (!plain)
        return failure(wrongKey);

    Result result;
    result.data = *plain;
    return result;
}

} // namespace PublicKeyEncryption
