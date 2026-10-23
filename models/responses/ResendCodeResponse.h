#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

// Resposta de POST /auth/2fa/resend
class ResendCodeResponse
{
public:
    QString destination;

    static ResendCodeResponse fromJson(const QJsonValue &value)
    {
        const QJsonObject json = value.toObject();
        ResendCodeResponse response;
        response.destination = json.value("destination").toString();
        return response;
    }
};
