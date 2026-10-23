#include "KeyStore.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

#include "security/Hash.h"
#include "security/Random.h"
#include "security/symmetric/Aes256Cbc.h"

namespace KeyStore {

namespace {

QString ensureDirectory(const QString &path)
{
    QDir().mkpath(path);
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    return path;
}

QString sanitize(const QString &name)
{
    QString value = name.toLower();
    value.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]")), QStringLiteral("_"));
    return value;
}

} // namespace

QString rootDirectory()
{
    return ensureDirectory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

QString keysDirectory()
{
    return ensureDirectory(QDir(rootDirectory()).filePath(QStringLiteral("keys")));
}

QString dataDirectory()
{
    return ensureDirectory(QDir(rootDirectory()).filePath(QStringLiteral("data")));
}

QString clientPublicKeyPath()
{
    return QDir(keysDirectory()).filePath(QStringLiteral("client_public_key.pem"));
}

QString serverPublicKeyPath(const QString &service)
{
    return QDir(keysDirectory())
        .filePath(QStringLiteral("server_%1_public_key.pem").arg(sanitize(service)));
}

bool saveClientPublicKey(const QByteArray &pem)
{
    return writeFile(clientPublicKeyPath(), pem);
}

QByteArray loadClientPublicKey()
{
    return readFile(clientPublicKeyPath());
}

bool saveServerPublicKey(const QString &service, const QByteArray &pem)
{
    return writeFile(serverPublicKeyPath(service), pem);
}

QByteArray loadServerPublicKey(const QString &service)
{
    return readFile(serverPublicKeyPath(service));
}

QByteArray loadOrCreateSalt(const QString &login)
{
    const QString name = QStringLiteral("local_%1.salt")
                             .arg(Hash::sha256Hex(login.trimmed().toLower().toUtf8()).left(16));
    const QString path = QDir(keysDirectory()).filePath(name);

    QByteArray salt = readFile(path);
    if (salt.size() == Aes256Cbc::SaltSize)
        return salt;

    salt = Random::bytes(Aes256Cbc::SaltSize);
    writeFile(path, salt);
    return salt;
}

bool writeFile(const QString &path, const QByteArray &data)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("KeyStore: não foi possível gravar %s", qPrintable(path));
        return false;
    }
    file.write(data);
    if (!file.commit())
        return false;

    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    return file.readAll();
}

} // namespace KeyStore
