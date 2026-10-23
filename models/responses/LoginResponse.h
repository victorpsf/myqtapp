#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

// Resposta de POST /auth/login
class LoginResponse
{
public:
    QString twoFactorToken;
    QString destination; // ex.: "v***@email.com"

    static LoginResponse fromJson(const QJsonValue &value)
    {
        const QJsonObject json = value.toObject();
        LoginResponse response;
        response.twoFactorToken = json.value("twoFactorToken").toString();
        response.destination = json.value("destination").toString();
        return response;
    }
};
