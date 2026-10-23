#include "Env.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

QHash<QString, QString> Env::s_values;

bool Env::load(const QString &filePath)
{
    if (!filePath.isEmpty())
        return parseFile(filePath);

    QStringList candidates;

    // AppImage: um .env ao lado do arquivo .AppImage substitui o embutido
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    if (!appImage.isEmpty())
        candidates.append(QFileInfo(appImage).dir().filePath(".env"));

    // Por usuário (útil no .deb, onde a pasta de instalação é só leitura)
    candidates.append(
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath(".env"));

    // Junto do executável (desenvolvimento, Windows e Mac)
    const QDir appDir(QCoreApplication::applicationDirPath());
    candidates.append(appDir.filePath(".env"));

    // Instalação Linux (.deb e AppImage): <prefixo>/share/myqtapp/.env
    candidates.append(appDir.filePath("../share/myqtapp/.env"));

    candidates.append(QDir::current().filePath(".env"));

    for (const QString &candidate : candidates) {
        if (QFile::exists(candidate))
            return parseFile(candidate);
    }

    qWarning("Env: arquivo .env não encontrado");
    return false;
}

QString Env::get(const QString &key, const QString &defaultValue)
{
    const QByteArray systemValue = qgetenv(key.toUtf8().constData());
    if (!systemValue.isEmpty())
        return QString::fromUtf8(systemValue);

    return s_values.value(key, defaultValue);
}

bool Env::parseFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Env: não foi possível abrir %s", qPrintable(filePath));
        return false;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        const int separator = line.indexOf('=');
        if (separator <= 0)
            continue;

        const QString key = line.left(separator).trimmed();
        QString value = line.mid(separator + 1).trimmed();

        if (value.size() >= 2
            && ((value.startsWith('"') && value.endsWith('"'))
                || (value.startsWith('\'') && value.endsWith('\'')))) {
            value = value.mid(1, value.size() - 2);
        }

        s_values.insert(key, value);
    }

    return true;
}
