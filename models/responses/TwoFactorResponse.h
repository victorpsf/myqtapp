#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include "models/entity/User.h"

// Resposta de POST /auth/2fa/verify
class TwoFactorResponse
{
public:
    QString accessToken;
    QString refreshToken;
    int expiresIn = 0;
    User user;

    static TwoFactorResponse fromJson(const QJsonValue &value)
    {
        const QJsonObject json = value.toObject();
        TwoFactorResponse response;
        response.accessToken = json.value("accessToken").toString();
        response.refreshToken = json.value("refreshToken").toString();
        response.expiresIn = json.value("expiresIn").toInt();
        response.user = User::fromJson(json.value("user"));
        return response;
    }
};
