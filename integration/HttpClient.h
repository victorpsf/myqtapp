#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>

#include "models/responses/IntegrationResponse.h"

class QNetworkAccessManager;

// Cliente HTTP base. Cada microserviço tem a sua própria instância
// apontando para o seu domínio (definido no .env).
class HttpClient : public QObject
{
    Q_OBJECT

public:
    explicit HttpClient(const QString &baseUrl, QObject *parent = nullptr);

    QString baseUrl() const { return m_baseUrl; }

    void setBearerToken(const QString &token) { m_bearerToken = token; }

    template <typename T>
    void get(const QString &path, std::function<void(const IntegrationResponse<T> &)> callback)
    {
        send("GET", path, QJsonObject(), wrap<T>(std::move(callback)));
    }

    template <typename T>
    void post(const QString &path, const QJsonObject &body,
              std::function<void(const IntegrationResponse<T> &)> callback)
    {
        send("POST", path, body, wrap<T>(std::move(callback)));
    }

    template <typename T>
    void put(const QString &path, const QJsonObject &body,
             std::function<void(const IntegrationResponse<T> &)> callback)
    {
        send("PUT", path, body, wrap<T>(std::move(callback)));
    }

private:
    // json: corpo da resposta (se for um objeto JSON válido)
    // transportError: erro de rede/HTTP quando não há "error" no corpo
    using RawCallback = std::function<void(const QJsonObject *json, const QString &transportError)>;

    template <typename T>
    static RawCallback wrap(std::function<void(const IntegrationResponse<T> &)> callback)
    {
        return [callback](const QJsonObject *json, const QString &transportError) {
            if (!callback)
                return;

            if (!json) {
                callback(IntegrationResponse<T>::failure(transportError));
                return;
            }

            IntegrationResponse<T> response = IntegrationResponse<T>::fromJson(*json);
            if (!response.hasError() && !transportError.isEmpty())
                response.error = transportError;

            callback(response);
        };
    }

    void send(const QByteArray &method, const QString &path, const QJsonObject &body,
              RawCallback callback);

    QNetworkAccessManager *m_network;
    QString m_baseUrl;
    QString m_bearerToken;
};
