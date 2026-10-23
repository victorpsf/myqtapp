#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

#include <atomic>

// Gera pares de chaves assimétricas e exporta em PEM:
//   pública: SubjectPublicKeyInfo  |  privada: PKCS#8 (com senha: AES-256-CBC)
namespace AsymmetricKeyGenerator {

enum class Algorithm { Rsa, Ec, Ed25519, Ed448 };

struct AlgorithmInfo
{
    Algorithm algorithm;
    QString name;
    QString description;
};

struct Options
{
    Algorithm algorithm = Algorithm::Rsa;
    int rsaBits = 4096;
    QString curve = QStringLiteral("P-256");
};

struct KeyPair
{
    QByteArray publicPem;
    QByteArray privatePem;
    QString error;

    bool isValid() const { return error.isEmpty() && !publicPem.isEmpty() && !privatePem.isEmpty(); }
};

QList<AlgorithmInfo> algorithms();
QList<int> rsaSizes();
QStringList curves();

// Nome curto para arquivos, ex.: "rsa4096", "ec_p256", "ed25519"
QString shortName(const Options &options);
QString displayName(const Options &options);

// Bloqueante (RSA grande leva tempo). `cancel` aborta a geração RSA em andamento.
KeyPair generate(const Options &options, const QByteArray &passphrase,
                 const std::atomic_bool *cancel = nullptr);

} // namespace AsymmetricKeyGenerator
