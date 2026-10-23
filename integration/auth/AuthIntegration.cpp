#include "AuthIntegration.h"

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include "config/AppConfig.h"
#include "security/asymmetric/Rsa.h"
#include "security/asymmetric/SecureEnvelope.h"
#include "models/entity/UserProfile.h"

// MOCK: enquanto o microserviço auth não está disponível, os métodos
// devolvem respostas fixas. Para usar a API real, descomente as linhas
// "post<...>" e remova o bloco de mock de cada método.
//
// Valores de teste:
//   usuário/e-mail  -> qualquer um; "erro" retorna erro da API
//   senha           -> qualquer uma; "errada" retorna erro da API
//   código 2FA      -> "123456" é válido; qualquer outro retorna erro
//   handshake/perfil -> o mock simula o servidor: gera a própria chave RSA,
//                       guarda a pública do cliente e devolve o perfil criptografado
namespace {

constexpr int MockDelayMs = 400;
const QString MockValidCode = QStringLiteral("123456");

// Só o mock usa 2048 bits, para não esperar duas gerações de 8192 ao testar.
// O cliente continua usando RsaKey::DefaultBits (8192).
constexpr int MockServerKeyBits = 2048;

struct MockServer
{
    QString login;
    RsaKey serverKey;
    RsaKey clientKey;
};

MockServer &mockServer()
{
    static MockServer server;
    return server;
}

template <typename T>
IntegrationResponse<T> mockSuccess(const T &data)
{
    IntegrationResponse<T> response;
    response.data = data;
    return response;
}

} // namespace

AuthIntegration::AuthIntegration(QObject *parent)
    : HttpClient(AppConfig::authApiUrl(), parent)
{
}

void AuthIntegration::identify(const AuthenticationRequest &request,
                               Callback<IdentifyResponse> callback)
{
    // post<IdentifyResponse>("/auth/identify", request.toIdentifyJson(), std::move(callback));

    mockServer().login = request.login();

    IntegrationResponse<IdentifyResponse> response;
    if (request.login().compare("erro", Qt::CaseInsensitive) == 0) {
        response = IntegrationResponse<IdentifyResponse>::failure(
            tr("Usuário ou e-mail não cadastrado."));
    } else {
        IdentifyResponse data;
        data.exists = true;
        data.displayName = request.login();
        response = mockSuccess(data);
    }

    QTimer::singleShot(MockDelayMs, this, [callback, response]() { callback(response); });
}

void AuthIntegration::login(const AuthenticationRequest &request, Callback<LoginResponse> callback)
{
    // post<LoginResponse>("/auth/login", request.toLoginJson(), std::move(callback));

    IntegrationResponse<LoginResponse> response;
    if (request.password() == "errada") {
        response = IntegrationResponse<LoginResponse>::failure(tr("Senha incorreta."));
    } else {
        LoginResponse data;
        data.twoFactorToken = QStringLiteral("mock-2fa-token");
        data.destination = QStringLiteral("v***@email.com");
        response = mockSuccess(data);
    }

    QTimer::singleShot(MockDelayMs, this, [callback, response]() { callback(response); });
}

void AuthIntegration::verifyTwoFactor(const AuthenticationRequest &request,
                                      Callback<TwoFactorResponse> callback)
{
    // post<TwoFactorResponse>("/auth/2fa/verify", request.toVerifyTwoFactorJson(),
    //                         std::move(callback));

    IntegrationResponse<TwoFactorResponse> response;
    if (request.twoFactorCode() != MockValidCode) {
        response = IntegrationResponse<TwoFactorResponse>::failure(
            tr("Código inválido ou expirado."));
    } else {
        TwoFactorResponse data;
        data.accessToken = QStringLiteral("mock-access-token");
        data.refreshToken = QStringLiteral("mock-refresh-token");
        data.expiresIn = 3600;
        data.user.id = QStringLiteral("1");
        data.user.name = QStringLiteral("Usuário Teste");
        if (request.isEmail())
            data.user.email = request.login();
        else
            data.user.username = request.login();
        response = mockSuccess(data);
    }

    QTimer::singleShot(MockDelayMs, this, [callback, response]() { callback(response); });
}

void AuthIntegration::resendTwoFactor(const AuthenticationRequest &request,
                                      Callback<ResendCodeResponse> callback)
{
    // post<ResendCodeResponse>("/auth/2fa/resend", request.toResendTwoFactorJson(),
    //                          std::move(callback));
    Q_UNUSED(request);

    ResendCodeResponse data;
    data.destination = QStringLiteral("v***@email.com");
    const IntegrationResponse<ResendCodeResponse> response = mockSuccess(data);

    QTimer::singleShot(MockDelayMs, this, [callback, response]() { callback(response); });
}

void AuthIntegration::handshake(const HandshakeRequest &request,
                                Callback<HandshakeResponse> callback)
{
    // post<HandshakeResponse>("/auth/handshake", request.toJson(), std::move(callback));

    mockServer().clientKey = RsaKey::fromPublicPem(request.clientPublicKey.toUtf8());
    if (mockServer().clientKey.isNull()) {
        const auto response =
            IntegrationResponse<HandshakeResponse>::failure(tr("Chave pública do cliente inválida."));
        QTimer::singleShot(MockDelayMs, this, [callback, response]() { callback(response); });
        return;
    }

    const RsaKey existingKey = mockServer().serverKey;
    auto *watcher = new QFutureWatcher<RsaKey>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [watcher, callback]() {
        watcher->deleteLater();

        const RsaKey serverKey = watcher->result();
        if (serverKey.isNull()) {
            callback(IntegrationResponse<HandshakeResponse>::failure(
                tr("Falha ao gerar a chave do servidor (mock).")));
            return;
        }

        mockServer().serverKey = serverKey;

        HandshakeResponse data;
        data.serverPublicKey = QString::fromUtf8(serverKey.publicPem());
        callback(mockSuccess(data));
    });

    watcher->setFuture(QtConcurrent::run([existingKey]() {
        return existingKey.isNull() ? RsaKey::generate(MockServerKeyBits) : existingKey;
    }));
}

void AuthIntegration::getProfile(Callback<EncryptedPayload> callback)
{
    // get<EncryptedPayload>("/auth/profile", std::move(callback));

    const MockServer &server = mockServer();
    IntegrationResponse<EncryptedPayload> response;

    if (server.serverKey.isNull() || server.clientKey.isNull()) {
        response = IntegrationResponse<EncryptedPayload>::failure(tr("Handshake não realizado."));
    } else {
        const bool isEmail = server.login.contains('@');

        UserProfile profile;
        profile.id = QStringLiteral("1");
        profile.name = QStringLiteral("Usuário Teste");
        profile.username = isEmail ? QStringLiteral("usuario.teste") : server.login;
        profile.email = isEmail ? server.login : QStringLiteral("usuario.teste@email.com");
        profile.phone = QStringLiteral("(11) 98765-4321");
        profile.document = QStringLiteral("123.456.789-00");
        profile.role = QStringLiteral("Administrador");
        profile.company = QStringLiteral("Empresa Exemplo LTDA");
        profile.createdAt = QStringLiteral("2024-03-15T10:30:00Z");

        // Servidor cifra com a pública do cliente e assina com a própria privada
        const std::optional<SealedMessage> sealed = SecureEnvelope::seal(
            QJsonDocument(profile.toJson()).toJson(QJsonDocument::Compact), server.clientKey,
            server.serverKey);

        response = sealed ? mockSuccess(EncryptedPayload::fromSealed(*sealed))
                          : IntegrationResponse<EncryptedPayload>::failure(
                                tr("Falha ao criptografar o perfil (mock)."));
    }

    QTimer::singleShot(MockDelayMs, this, [callback, response]() { callback(response); });
}
