#include "ContentPackage.h"

#include <QFileInfo>
#include <QtEndian>

namespace ContentPackage {

namespace {

const QByteArray Magic = QByteArrayLiteral("MQAPKG");
constexpr quint8 Version = 1;
constexpr int HeaderSize = 6 + 1 + 2;

} // namespace

QByteArray pack(const Content &content)
{
    // só o nome, nunca o caminho; limitado ao campo de 2 bytes
    const QByteArray name = QFileInfo(content.fileName).fileName().toUtf8().left(0xFFFF);

    QByteArray packed;
    packed.reserve(HeaderSize + name.size() + content.data.size());
    packed.append(Magic);
    packed.append(char(Version));
    char nameSize[2];
    qToBigEndian(quint16(name.size()), nameSize);
    packed.append(nameSize, 2);
    packed.append(name);
    packed.append(content.data);
    return packed;
}

Content unpack(const QByteArray &packed)
{
    Content content;
    if (packed.size() < HeaderSize || !packed.startsWith(Magic)
        || quint8(packed.at(6)) != Version) {
        content.data = packed;
        return content;
    }

    const int nameSize = qFromBigEndian<quint16>(packed.constData() + 7);
    if (packed.size() < HeaderSize + nameSize) {
        content.data = packed;
        return content;
    }

    // remove qualquer caminho que venha no nome (ex.: "../../.bashrc")
    content.fileName =
        QFileInfo(QString::fromUtf8(packed.mid(HeaderSize, nameSize))).fileName();
    if (content.fileName == QLatin1String(".") || content.fileName == QLatin1String(".."))
        content.fileName = QStringLiteral("arquivo");
    content.data = packed.mid(HeaderSize + nameSize);
    return content;
}

} // namespace ContentPackage
