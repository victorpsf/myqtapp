#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <optional>

// Envelope padrão das respostas dos microserviços:
// { "error": "", "data": null }
//
// T precisa expor: static T fromJson(const QJsonValue &value)
template <typename T>
class IntegrationResponse
{
public:
    QString error;
    std::optional<T> data;

    bool hasError() const { return !error.isEmpty(); }
    bool isSuccess() const { return !hasError(); }

    static IntegrationResponse<T> fromJson(const QJsonObject &json)
    {
        IntegrationResponse<T> response;
        response.error = json.value("error").toString();

        const QJsonValue dataValue = json.value("data");
        if (!dataValue.isNull() && !dataValue.isUndefined())
            response.data = T::fromJson(dataValue);

        return response;
    }

    static IntegrationResponse<T> failure(const QString &message)
    {
        IntegrationResponse<T> response;
        response.error = message;
        return response;
    }
};
