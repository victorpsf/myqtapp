#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

// Resposta de POST /auth/handshake: chave pública do servidor (PEM)
class HandshakeResponse
{
public:
    QString serverPublicKey;

    static HandshakeResponse fromJson(const QJsonValue &value)
    {
        const QJsonObject json = value.toObject();
        HandshakeResponse response;
        response.serverPublicKey = json.value("publicKey").toString();
        return response;
    }
};
