#include "KeyToolsCommon.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QStringList>

#include "components/popups/Popup.h"
#include "security/Hash.h"
#include "security/KeyStore.h"

namespace KeyTools {

QString saveToChosenFolder(QWidget *parent, const QString &dialogTitle,
                           const QList<OutputFile> &files)
{
    // fora do QSettings padrão para não depender do organizationName
    QSettings settings(QDir(KeyStore::rootDirectory()).filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    const QString lastFolder = settings.value("keyTools/lastFolder", QDir::homePath()).toString();

    const QString folder = QFileDialog::getExistingDirectory(parent, dialogTitle, lastFolder);
    if (folder.isEmpty())
        return QString();

    settings.setValue("keyTools/lastFolder", folder);

    QStringList existing;
    for (const OutputFile &file : files) {
        if (QFile::exists(QDir(folder).filePath(file.first)))
            existing.append(file.first);
    }

    if (!existing.isEmpty()) {
        const bool overwrite = Popup::display(
            parent, {PopupType::Warning, QObject::tr("Substituir arquivos?"),
                     QObject::tr("Já existe na pasta:\n%1\n\nDeseja substituir?")
                         .arg(existing.join('\n')),
                     QObject::tr("Substituir"), QObject::tr("Cancelar")});
        if (!overwrite)
            return QString();
    }

    for (const OutputFile &file : files) {
        const QString path = QDir(folder).filePath(file.first);
        if (!KeyStore::writeFile(path, file.second)) {
            Popup::error(parent, QObject::tr("Não foi possível gravar %1.").arg(path));
            return QString();
        }
    }

    return folder;
}

QString timestamp()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
}

QString encryptedFileName(const QString &originalPath)
{
    const QFileInfo info(originalPath);
    const QString hash = Hash::sha256Hex(info.fileName().toUtf8());
    return info.suffix().isEmpty() ? hash : hash + '.' + info.suffix();
}

std::optional<QByteArray> readWholeFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QObject::tr("Não foi possível ler o arquivo:\n%1").arg(path);
        return std::nullopt;
    }
    return file.readAll();
}

std::optional<QByteArray> readKeyFile(const QString &path, QString *error)
{
    constexpr qint64 MaxKeyFileSize = 64 * 1024;
    if (QFileInfo(path).size() > MaxKeyFileSize) {
        if (error)
            *error = QObject::tr("O arquivo é grande demais para ser uma chave PEM.");
        return std::nullopt;
    }
    return readWholeFile(path, error);
}

std::optional<QByteArray> decodeBase64(const QString &text)
{
    QByteArray compact;
    compact.reserve(text.size());
    for (const QChar c : text) {
        if (!c.isSpace())
            compact.append(char(c.toLatin1()));
    }

    const auto decoded =
        QByteArray::fromBase64Encoding(compact, QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded || decoded.decoded.isEmpty())
        return std::nullopt;
    return decoded.decoded;
}

} // namespace KeyTools
