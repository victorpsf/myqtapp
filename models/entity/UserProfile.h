#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVariant>

// Perfil do usuário (vem criptografado de GET /auth/profile)
class UserProfile
{
public:
    QString id;
    QString name;
    QString username;
    QString email;
    QString phone;
    QString document;
    QString role;
    QString company;
    QString createdAt; // ISO 8601

    static UserProfile fromJson(const QJsonValue &value)
    {
        const QJsonObject json = value.toObject();
        UserProfile profile;
        profile.id = json.value("id").toVariant().toString();
        profile.name = json.value("name").toString();
        profile.username = json.value("username").toString();
        profile.email = json.value("email").toString();
        profile.phone = json.value("phone").toString();
        profile.document = json.value("document").toString();
        profile.role = json.value("role").toString();
        profile.company = json.value("company").toString();
        profile.createdAt = json.value("createdAt").toString();
        return profile;
    }

    QJsonObject toJson() const
    {
        return {
            {"id", id},
            {"name", name},
            {"username", username},
            {"email", email},
            {"phone", phone},
            {"document", document},
            {"role", role},
            {"company", company},
            {"createdAt", createdAt},
        };
    }
};
