#pragma once

#include <QObject>

#include <functional>

#include "integration/HttpClient.h"
#include "models/requests/AuthenticationRequest.h"
#include "models/requests/HandshakeRequest.h"
#include "models/responses/EncryptedPayload.h"
#include "models/responses/HandshakeResponse.h"
#include "models/responses/IdentifyResponse.h"
#include "models/responses/IntegrationResponse.h"
#include "models/responses/LoginResponse.h"
#include "models/responses/ResendCodeResponse.h"
#include "models/responses/TwoFactorResponse.h"

// Comunicação com o microserviço "auth" (AUTH_API_URL)
class AuthIntegration : public HttpClient
{
    Q_OBJECT

public:
    template <typename T>
    using Callback = std::function<void(const IntegrationResponse<T> &)>;

    // Nome usado para guardar a chave pública deste microserviço
    static inline const QString ServiceName = QStringLiteral("auth");

    explicit AuthIntegration(QObject *parent = nullptr);

    // Tela 1: valida o usuário/e-mail
    void identify(const AuthenticationRequest &request, Callback<IdentifyResponse> callback);

    // Tela 2: valida a senha e dispara o envio do código 2FA
    void login(const AuthenticationRequest &request, Callback<LoginResponse> callback);

    // Tela 3: valida o código 2FA e devolve a sessão
    void verifyTwoFactor(const AuthenticationRequest &request, Callback<TwoFactorResponse> callback);

    // Tela 3: reenvia o código 2FA
    void resendTwoFactor(const AuthenticationRequest &request, Callback<ResendCodeResponse> callback);

    // Troca de chaves: envia a pública do cliente e recebe a pública do servidor
    void handshake(const HandshakeRequest &request, Callback<HandshakeResponse> callback);

    // Perfil do usuário logado — resposta criptografada (EncryptedPayload)
    void getProfile(Callback<EncryptedPayload> callback);
};
