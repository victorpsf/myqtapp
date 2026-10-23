#pragma once

#include <QMainWindow>

#include "models/requests/AuthenticationRequest.h"
#include "models/responses/TwoFactorResponse.h"

class QStackedWidget;
class AuthIntegration;
class PersonalIntegration;
class LoginIdentifierScreen;
class LoginPasswordScreen;
class TwoFactorScreen;
class HomeScreen;

// Controla a navegação entre as telas do fluxo de autenticação
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void showIdentifier();
    void showPassword();
    void showTwoFactor();
    void showHome(const TwoFactorResponse &session);

    AuthenticationRequest m_authRequest;
    TwoFactorResponse m_session;

    AuthIntegration *m_auth;
    PersonalIntegration *m_personal;

    QStackedWidget *m_stack;
    LoginIdentifierScreen *m_identifierScreen;
    LoginPasswordScreen *m_passwordScreen;
    TwoFactorScreen *m_twoFactorScreen;
    HomeScreen *m_homeScreen;
};
