#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

// Resposta de POST /auth/identify
class IdentifyResponse
{
public:
    bool exists = false;
    QString displayName;

    static IdentifyResponse fromJson(const QJsonValue &value)
    {
        const QJsonObject json = value.toObject();
        IdentifyResponse response;
        response.exists = json.value("exists").toBool();
        response.displayName = json.value("displayName").toString();
        return response;
    }
};
