#pragma once

#include <QByteArray>
#include <QString>

namespace OpenSsl {

// Esvazia a fila de erros do OpenSSL e devolve a mensagem
QString lastError();

// Sobrescreve o buffer com zeros (dados sensíveis: senhas, chaves)
void cleanse(QByteArray &data);

inline const unsigned char *bytes(const QByteArray &data)
{
    return reinterpret_cast<const unsigned char *>(data.constData());
}

inline unsigned char *bytes(QByteArray &data)
{
    return reinterpret_cast<unsigned char *>(data.data());
}

} // namespace OpenSsl
