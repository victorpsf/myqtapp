#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVariant>

class User
{
public:
    QString id;
    QString name;
    QString username;
    QString email;

    static User fromJson(const QJsonValue &value)
    {
        const QJsonObject json = value.toObject();
        User user;
        user.id = json.value("id").toVariant().toString();
        user.name = json.value("name").toString();
        user.username = json.value("username").toString();
        user.email = json.value("email").toString();
        return user;
    }
};
