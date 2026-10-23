#include "HttpClient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "config/AppConfig.h"

HttpClient::HttpClient(const QString &baseUrl, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_baseUrl(baseUrl)
{
    while (m_baseUrl.endsWith('/'))
        m_baseUrl.chop(1);
}

void HttpClient::send(const QByteArray &method, const QString &path, const QJsonObject &body,
                      RawCallback callback)
{
    if (m_baseUrl.isEmpty()) {
        callback(nullptr, tr("Domínio do serviço não configurado no .env"));
        return;
    }

    const QString normalizedPath = path.startsWith('/') ? path : '/' + path;
    QNetworkRequest request(QUrl(m_baseUrl + normalizedPath));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(AppConfig::httpTimeoutMs());

    if (!m_bearerToken.isEmpty())
        request.setRawHeader("Authorization", "Bearer " + m_bearerToken.toUtf8());

    QNetworkReply *reply = nullptr;
    if (method == "GET") {
        reply = m_network->get(request);
    } else {
        const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
        reply = m_network->sendCustomRequest(request, method, payload);
    }

    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();

        const QByteArray raw = reply->readAll();
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(raw, &parseError);
        const bool hasJson = parseError.error == QJsonParseError::NoError && document.isObject();

        QString transportError;
        if (reply->error() != QNetworkReply::NoError) {
            transportError = reply->error() == QNetworkReply::OperationCanceledError
                                 ? tr("Tempo de resposta do servidor esgotado")
                                 : reply->errorString();
        } else if (!hasJson) {
            transportError = tr("Resposta inválida do servidor");
        }

        if (hasJson) {
            const QJsonObject json = document.object();
            callback(&json, transportError);
        } else {
            callback(nullptr, transportError);
        }
    });
}
