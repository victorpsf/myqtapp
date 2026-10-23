#include "Hash.h"

#include <QCryptographicHash>
#include <QFile>

namespace Hash {

namespace {

QCryptographicHash::Algorithm toQt(Algorithm algorithm)
{
    switch (algorithm) {
    case Algorithm::Md5:         return QCryptographicHash::Md5;
    case Algorithm::Sha1:        return QCryptographicHash::Sha1;
    case Algorithm::Sha224:      return QCryptographicHash::Sha224;
    case Algorithm::Sha256:      return QCryptographicHash::Sha256;
    case Algorithm::Sha384:      return QCryptographicHash::Sha384;
    case Algorithm::Sha512:      return QCryptographicHash::Sha512;
    case Algorithm::Sha3_256:    return QCryptographicHash::Sha3_256;
    case Algorithm::Sha3_512:    return QCryptographicHash::Sha3_512;
    case Algorithm::Blake2b_512: return QCryptographicHash::Blake2b_512;
    }
    return QCryptographicHash::Sha256;
}

} // namespace

QList<AlgorithmInfo> algorithms()
{
    return {
        {Algorithm::Sha256, QStringLiteral("SHA-256"), false},
        {Algorithm::Sha512, QStringLiteral("SHA-512"), false},
        {Algorithm::Sha384, QStringLiteral("SHA-384"), false},
        {Algorithm::Sha224, QStringLiteral("SHA-224"), false},
        {Algorithm::Sha3_256, QStringLiteral("SHA3-256"), false},
        {Algorithm::Sha3_512, QStringLiteral("SHA3-512"), false},
        {Algorithm::Blake2b_512, QStringLiteral("BLAKE2b-512"), false},
        {Algorithm::Sha1, QStringLiteral("SHA-1"), true},
        {Algorithm::Md5, QStringLiteral("MD5"), true},
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

QByteArray hash(Algorithm algorithm, const QByteArray &data)
{
    return QCryptographicHash::hash(data, toQt(algorithm));
}

std::optional<QByteArray> hashFile(Algorithm algorithm, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;

    QCryptographicHash hasher(toQt(algorithm));
    if (!hasher.addData(&file))
        return std::nullopt;

    return hasher.result();
}

QByteArray sha256(const QByteArray &data)
{
    return hash(Algorithm::Sha256, data);
}

QByteArray sha512(const QByteArray &data)
{
    return hash(Algorithm::Sha512, data);
}

QString sha256Hex(const QByteArray &data)
{
    return QString::fromLatin1(sha256(data).toHex());
}

QString sha512Hex(const QByteArray &data)
{
    return QString::fromLatin1(sha512(data).toHex());
}

} // namespace Hash
