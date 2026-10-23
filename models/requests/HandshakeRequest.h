#pragma once

#include <QJsonObject>
#include <QString>

// Envia a chave pública do cliente (PEM) para o servidor
class HandshakeRequest
{
public:
    QString clientPublicKey;

    QJsonObject toJson() const
    {
        return {
            {"publicKey", clientPublicKey},
        };
    }
};
