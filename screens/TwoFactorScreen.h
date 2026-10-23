#pragma once

#include <QWidget>

#include "models/responses/TwoFactorResponse.h"

class AuthIntegration;
class AuthenticationRequest;
class FormCard;
class InputField;

class TwoFactorScreen : public QWidget
{
    Q_OBJECT

public:
    static constexpr int CodeLength = 6;

    TwoFactorScreen(AuthIntegration *auth, AuthenticationRequest *request,
                    QWidget *parent = nullptr);

    void reset();

signals:
    void verified(const TwoFactorResponse &session);

private:
    void verify();
    void resend();

    AuthIntegration *m_auth;
    AuthenticationRequest *m_request;
    FormCard *m_card;
    InputField *m_code;
};
