#include "OpenSsl.h"

#include <QStringList>

#include <openssl/crypto.h>
#include <openssl/err.h>

namespace OpenSsl {

QString lastError()
{
    QStringList messages;
    unsigned long code = 0;
    while ((code = ERR_get_error()) != 0) {
        char buffer[256];
        ERR_error_string_n(code, buffer, sizeof(buffer));
        messages.append(QString::fromLatin1(buffer));
    }
    return messages.isEmpty() ? QStringLiteral("erro desconhecido do OpenSSL")
                              : messages.join(QStringLiteral("; "));
}

void cleanse(QByteArray &data)
{
    if (!data.isEmpty())
        OPENSSL_cleanse(data.data(), size_t(data.size()));
    data.clear();
}

} // namespace OpenSsl
