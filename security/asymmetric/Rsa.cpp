#include "Rsa.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "security/OpenSsl.h"

namespace {

using PkeyCtx = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
using MdCtx = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
using Bio = std::unique_ptr<BIO, decltype(&BIO_free)>;

constexpr int OaepOverhead = 2 * 32 + 2; // 2 * tamanho do SHA-256 + 2

std::shared_ptr<evp_pkey_st> wrap(EVP_PKEY *key)
{
    return std::shared_ptr<evp_pkey_st>(key, EVP_PKEY_free);
}

Bio memoryBio()
{
    return Bio(BIO_new(BIO_s_mem()), BIO_free);
}

Bio memoryBio(const QByteArray &data)
{
    return Bio(BIO_new_mem_buf(data.constData(), int(data.size())), BIO_free);
}

QByteArray readBio(BIO *bio)
{
    char *data = nullptr;
    const long length = BIO_get_mem_data(bio, &data);
    return QByteArray(data, int(length));
}

bool configureOaep(EVP_PKEY_CTX *ctx)
{
    return EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) > 0
           && EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) > 0
           && EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) > 0;
}

bool configurePss(EVP_PKEY_CTX *ctx)
{
    return EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PSS_PADDING) > 0
           && EVP_PKEY_CTX_set_rsa_pss_saltlen(ctx, RSA_PSS_SALTLEN_DIGEST) > 0;
}

// Chamado pelo OpenSSL durante a busca dos primos: retornar 0 aborta a geração
int keygenProgress(EVP_PKEY_CTX *ctx)
{
    const auto *cancel = static_cast<const std::atomic_bool *>(EVP_PKEY_CTX_get_app_data(ctx));
    return cancel && cancel->load() ? 0 : 1;
}

void warn(const char *operation)
{
    qWarning("RsaKey::%s: %s", operation, qPrintable(OpenSsl::lastError()));
}

} // namespace

RsaKey::RsaKey(std::shared_ptr<evp_pkey_st> key, bool hasPrivateKey)
    : m_key(std::move(key))
    , m_hasPrivateKey(hasPrivateKey)
{
}

RsaKey RsaKey::generate(int bits, const std::atomic_bool *cancel)
{
    PkeyCtx ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), EVP_PKEY_CTX_free);
    EVP_PKEY *key = nullptr;

    if (!ctx || EVP_PKEY_keygen_init(ctx.get()) <= 0
        || EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), bits) <= 0) {
        warn("generate");
        return RsaKey();
    }

    if (cancel) {
        EVP_PKEY_CTX_set_app_data(ctx.get(), const_cast<std::atomic_bool *>(cancel));
        EVP_PKEY_CTX_set_cb(ctx.get(), keygenProgress);
    }

    if (EVP_PKEY_keygen(ctx.get(), &key) <= 0) {
        if (cancel && cancel->load())
            OpenSsl::lastError(); // cancelado: não é erro
        else
            warn("generate");
        return RsaKey();
    }

    return RsaKey(wrap(key), true);
}

RsaKey RsaKey::fromPublicPem(const QByteArray &pem)
{
    Bio bio = memoryBio(pem);
    EVP_PKEY *key = bio ? PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr) : nullptr;
    if (!key) {
        warn("fromPublicPem");
        return RsaKey();
    }
    return RsaKey(wrap(key), false);
}

RsaKey RsaKey::fromPrivatePem(const QByteArray &pem, const QByteArray &passphrase)
{
    Bio bio = memoryBio(pem);
    void *password = passphrase.isEmpty() ? nullptr : const_cast<char *>(passphrase.constData());
    EVP_PKEY *key = bio ? PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, password) : nullptr;
    if (!key) {
        warn("fromPrivatePem");
        return RsaKey();
    }
    return RsaKey(wrap(key), true);
}

int RsaKey::bits() const
{
    return m_key ? EVP_PKEY_bits(m_key.get()) : 0;
}

int RsaKey::maxEncryptSize() const
{
    return m_key ? EVP_PKEY_size(m_key.get()) - OaepOverhead : 0;
}

QByteArray RsaKey::publicPem() const
{
    Bio bio = memoryBio();
    if (!m_key || !bio || PEM_write_bio_PUBKEY(bio.get(), m_key.get()) != 1) {
        warn("publicPem");
        return QByteArray();
    }
    return readBio(bio.get());
}

QByteArray RsaKey::privatePem(const QByteArray &passphrase) const
{
    Bio bio = memoryBio();
    if (!m_key || !m_hasPrivateKey || !bio)
        return QByteArray();

    const EVP_CIPHER *cipher = passphrase.isEmpty() ? nullptr : EVP_aes_256_cbc();
    void *password = passphrase.isEmpty() ? nullptr : const_cast<char *>(passphrase.constData());

    if (PEM_write_bio_PKCS8PrivateKey(bio.get(), m_key.get(), cipher, nullptr, 0, nullptr, password)
        != 1) {
        warn("privatePem");
        return QByteArray();
    }
    return readBio(bio.get());
}

QByteArray RsaKey::encrypt(const QByteArray &plain) const
{
    if (!m_key || plain.size() > maxEncryptSize())
        return QByteArray();

    PkeyCtx ctx(EVP_PKEY_CTX_new(m_key.get(), nullptr), EVP_PKEY_CTX_free);
    size_t length = 0;

    if (!ctx || EVP_PKEY_encrypt_init(ctx.get()) <= 0 || !configureOaep(ctx.get())
        || EVP_PKEY_encrypt(ctx.get(), nullptr, &length, OpenSsl::bytes(plain), size_t(plain.size()))
               <= 0) {
        warn("encrypt");
        return QByteArray();
    }

    QByteArray cipher(int(length), Qt::Uninitialized);
    if (EVP_PKEY_encrypt(ctx.get(), OpenSsl::bytes(cipher), &length, OpenSsl::bytes(plain),
                         size_t(plain.size()))
        <= 0) {
        warn("encrypt");
        return QByteArray();
    }

    cipher.resize(int(length));
    return cipher;
}

std::optional<QByteArray> RsaKey::decrypt(const QByteArray &cipher) const
{
    if (!m_key || !m_hasPrivateKey)
        return std::nullopt;

    PkeyCtx ctx(EVP_PKEY_CTX_new(m_key.get(), nullptr), EVP_PKEY_CTX_free);
    size_t length = 0;

    if (!ctx || EVP_PKEY_decrypt_init(ctx.get()) <= 0 || !configureOaep(ctx.get())
        || EVP_PKEY_decrypt(ctx.get(), nullptr, &length, OpenSsl::bytes(cipher),
                            size_t(cipher.size()))
               <= 0) {
        warn("decrypt");
        return std::nullopt;
    }

    QByteArray plain(int(length), Qt::Uninitialized);
    if (EVP_PKEY_decrypt(ctx.get(), OpenSsl::bytes(plain), &length, OpenSsl::bytes(cipher),
                         size_t(cipher.size()))
        <= 0) {
        warn("decrypt");
        return std::nullopt;
    }

    plain.resize(int(length));
    return plain;
}

QByteArray RsaKey::sign(const QByteArray &data) const
{
    if (!m_key || !m_hasPrivateKey)
        return QByteArray();

    MdCtx md(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    EVP_PKEY_CTX *pctx = nullptr; // pertence ao md
    size_t length = 0;

    if (!md || EVP_DigestSignInit(md.get(), &pctx, EVP_sha256(), nullptr, m_key.get()) <= 0
        || !configurePss(pctx)
        || EVP_DigestSign(md.get(), nullptr, &length, OpenSsl::bytes(data), size_t(data.size()))
               <= 0) {
        warn("sign");
        return QByteArray();
    }

    QByteArray signature(int(length), Qt::Uninitialized);
    if (EVP_DigestSign(md.get(), OpenSsl::bytes(signature), &length, OpenSsl::bytes(data),
                       size_t(data.size()))
        <= 0) {
        warn("sign");
        return QByteArray();
    }

    signature.resize(int(length));
    return signature;
}

bool RsaKey::verify(const QByteArray &data, const QByteArray &signature) const
{
    if (!m_key || signature.isEmpty())
        return false;

    MdCtx md(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    EVP_PKEY_CTX *pctx = nullptr;

    if (!md || EVP_DigestVerifyInit(md.get(), &pctx, EVP_sha256(), nullptr, m_key.get()) <= 0
        || !configurePss(pctx)) {
        warn("verify");
        return false;
    }

    const int result = EVP_DigestVerify(md.get(), OpenSsl::bytes(signature),
                                        size_t(signature.size()), OpenSsl::bytes(data),
                                        size_t(data.size()));
    if (result != 1)
        OpenSsl::lastError();
    return result == 1;
}
