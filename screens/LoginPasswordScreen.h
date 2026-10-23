#pragma once

#include <QWidget>

class AuthIntegration;
class AuthenticationRequest;
class FormCard;
class InputField;
class TextLabel;

class LoginPasswordScreen : public QWidget
{
    Q_OBJECT

public:
    LoginPasswordScreen(AuthIntegration *auth, AuthenticationRequest *request,
                        QWidget *parent = nullptr);

    void reset();

signals:
    void loggedIn();
    void back();

private:
    void submit();
    void goBack();

    AuthIntegration *m_auth;
    AuthenticationRequest *m_request;
    FormCard *m_card;
    TextLabel *m_loginLabel;
    InputField *m_password;
};
