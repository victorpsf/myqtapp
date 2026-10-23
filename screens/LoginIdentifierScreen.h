#pragma once

#include <QWidget>

class AuthIntegration;
class AuthenticationRequest;
class FormCard;
class InputField;

class LoginIdentifierScreen : public QWidget
{
    Q_OBJECT

public:
    LoginIdentifierScreen(AuthIntegration *auth, AuthenticationRequest *request,
                          QWidget *parent = nullptr);

    void reset();

signals:
    void identified();

private:
    void submit();

    AuthIntegration *m_auth;
    AuthenticationRequest *m_request;
    FormCard *m_card;
    InputField *m_login;
};
