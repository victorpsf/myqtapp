#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include <optional>

namespace Hash {

enum class Algorithm { Md5, Sha1, Sha224, Sha256, Sha384, Sha512, Sha3_256, Sha3_512, Blake2b_512 };

struct AlgorithmInfo
{
    Algorithm algorithm;
    QString name;
    bool weak; // não usar para segurança (colisões conhecidas)
};

QList<AlgorithmInfo> algorithms();
QString name(Algorithm algorithm);

QByteArray hash(Algorithm algorithm, const QByteArray &data);

// Lê o arquivo em blocos (não carrega tudo na memória)
std::optional<QByteArray> hashFile(Algorithm algorithm, const QString &path);

QByteArray sha256(const QByteArray &data);
QByteArray sha512(const QByteArray &data);

QString sha256Hex(const QByteArray &data);
QString sha512Hex(const QByteArray &data);

} // namespace Hash
