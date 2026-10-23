#include "AsymmetricKeyGenerator.h"

#include <QObject>

#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <memory>

#include "security/OpenSsl.h"

namespace AsymmetricKeyGenerator {

namespace {

using PkeyCtx = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
using Pkey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using Bio = std::unique_ptr<BIO, decltype(&BIO_free)>;

int curveNid(const QString &curve)
{
    if (curve == QLatin1String("P-384"))
        return NID_secp384r1;
    if (curve == QLatin1String("P-521"))
        return NID_secp521r1;
    if (curve == QLatin1String("secp256k1"))
        return NID_secp256k1;
    return NID_X9_62_prime256v1; // P-256
}

int keyType(Algorithm algorithm)
{
    switch (algorithm) {
    case Algorithm::Rsa:     return EVP_PKEY_RSA;
    case Algorithm::Ec:      return EVP_PKEY_EC;
    case Algorithm::Ed25519: return EVP_PKEY_ED25519;
    case Algorithm::Ed448:   return EVP_PKEY_ED448;
    }
    return EVP_PKEY_RSA;
}

int progress(EVP_PKEY_CTX *ctx)
{
    const auto *cancel = static_cast<const std::atomic_bool *>(EVP_PKEY_CTX_get_app_data(ctx));
    return cancel && cancel->load() ? 0 : 1;
}

QByteArray readBio(BIO *bio)
{
    char *data = nullptr;
    const long length = BIO_get_mem_data(bio, &data);
    return QByteArray(data, int(length));
}

KeyPair failure(const QString &message)
{
    KeyPair pair;
    pair.error = message + QStringLiteral(" (") + OpenSsl::lastError() + ')';
    return pair;
}

} // namespace

QList<AlgorithmInfo> algorithms()
{
    return {
        {Algorithm::Rsa, QStringLiteral("RSA"),
         QObject::tr("Criptografia e assinatura. O mais compatível.")},
        {Algorithm::Ec, QStringLiteral("ECDSA (curva elíptica)"),
         QObject::tr("Assinatura e criptografia (via ECDH). Chaves pequenas.")},
        {Algorithm::Ed25519, QStringLiteral("Ed25519 (EdDSA)"),
         QObject::tr("Somente assinatura (não serve para criptografar).")},
        {Algorithm::Ed448, QStringLiteral("Ed448 (EdDSA)"),
         QObject::tr("Somente assinatura, com margem maior que o Ed25519.")},
    };
}

QList<int> rsaSizes()
{
    return {512, 1024, 2048, 4096, 8192};
}

QStringList curves()
{
    return {QStringLiteral("P-256"), QStringLiteral("P-384"), QStringLiteral("P-521"),
            QStringLiteral("secp256k1")};
}

QString shortName(const Options &options)
{
    switch (options.algorithm) {
    case Algorithm::Rsa:     return QStringLiteral("rsa%1").arg(options.rsaBits);
    case Algorithm::Ec:      return QStringLiteral("ec_%1").arg(options.curve.toLower().remove('-'));
    case Algorithm::Ed25519: return QStringLiteral("ed25519");
    case Algorithm::Ed448:   return QStringLiteral("ed448");
    }
    return QStringLiteral("key");
}

QString displayName(const Options &options)
{
    switch (options.algorithm) {
    case Algorithm::Rsa:     return QStringLiteral("RSA %1 bits").arg(options.rsaBits);
    case Algorithm::Ec:      return QStringLiteral("ECDSA %1").arg(options.curve);
    case Algorithm::Ed25519: return QStringLiteral("Ed25519");
    case Algorithm::Ed448:   return QStringLiteral("Ed448");
    }
    return QString();
}

KeyPair generate(const Options &options, const QByteArray &passphrase,
                 const std::atomic_bool *cancel)
{
    PkeyCtx ctx(EVP_PKEY_CTX_new_id(keyType(options.algorithm), nullptr), EVP_PKEY_CTX_free);
    if (!ctx || EVP_PKEY_keygen_init(ctx.get()) <= 0)
        return failure(QObject::tr("Algoritmo não suportado pelo OpenSSL instalado."));

    if (options.algorithm == Algorithm::Rsa
        && EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), options.rsaBits) <= 0)
        return failure(QObject::tr("Tamanho de chave RSA inválido."));

    if (options.algorithm == Algorithm::Ec
        && EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx.get(), curveNid(options.curve)) <= 0)
        return failure(QObject::tr("Curva elíptica inválida."));

    if (cancel) {
        EVP_PKEY_CTX_set_app_data(ctx.get(), const_cast<std::atomic_bool *>(cancel));
        EVP_PKEY_CTX_set_cb(ctx.get(), progress);
    }

    EVP_PKEY *raw = nullptr;
    if (EVP_PKEY_keygen(ctx.get(), &raw) <= 0) {
        if (cancel && cancel->load()) {
            OpenSsl::lastError();
            KeyPair cancelled;
            cancelled.error = QObject::tr("Geração cancelada.");
            return cancelled;
        }
        return failure(QObject::tr("Falha ao gerar as chaves."));
    }
    Pkey key(raw, EVP_PKEY_free);

    Bio publicBio(BIO_new(BIO_s_mem()), BIO_free);
    Bio privateBio(BIO_new(BIO_s_mem()), BIO_free);
    if (!publicBio || !privateBio || PEM_write_bio_PUBKEY(publicBio.get(), key.get()) != 1)
        return failure(QObject::tr("Falha ao exportar a chave pública."));

    const EVP_CIPHER *cipher = passphrase.isEmpty() ? nullptr : EVP_aes_256_cbc();
    void *password = passphrase.isEmpty() ? nullptr : const_cast<char *>(passphrase.constData());
    if (PEM_write_bio_PKCS8PrivateKey(privateBio.get(), key.get(), cipher, nullptr, 0, nullptr,
                                      password)
        != 1)
        return failure(QObject::tr("Falha ao exportar a chave privada."));

    KeyPair pair;
    pair.publicPem = readBio(publicBio.get());
    pair.privatePem = readBio(privateBio.get());
    return pair;
}

} // namespace AsymmetricKeyGenerator
