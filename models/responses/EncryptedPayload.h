#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include "security/asymmetric/SecureEnvelope.h"

// Envelope criptografado trafegado nas mensagens críticas (campos em base64):
// { "key": "...", "data": "...", "signature": "..." }
class EncryptedPayload
{
public:
    QString key;
    QString data;
    QString signature;

    static EncryptedPayload fromJson(const QJsonValue &value)
    {
        const QJsonObject json = value.toObject();
        EncryptedPayload payload;
        payload.key = json.value("key").toString();
        payload.data = json.value("data").toString();
        payload.signature = json.value("signature").toString();
        return payload;
    }

    QJsonObject toJson() const
    {
        return {
            {"key", key},
            {"data", data},
            {"signature", signature},
        };
    }

    static EncryptedPayload fromSealed(const SealedMessage &message)
    {
        EncryptedPayload payload;
        payload.key = QString::fromLatin1(message.key.toBase64());
        payload.data = QString::fromLatin1(message.data.toBase64());
        payload.signature = QString::fromLatin1(message.signature.toBase64());
        return payload;
    }

    SealedMessage toSealed() const
    {
        return {
            QByteArray::fromBase64(key.toLatin1()),
            QByteArray::fromBase64(data.toLatin1()),
            QByteArray::fromBase64(signature.toLatin1()),
        };
    }
};
