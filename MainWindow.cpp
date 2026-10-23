#include "MainWindow.h"

#include <QStackedWidget>

#include "components/popups/Popup.h"
#include "integration/auth/AuthIntegration.h"
#include "integration/personal/PersonalIntegration.h"
#include "screens/HomeScreen.h"
#include "security/SecuritySession.h"
#include "screens/LoginIdentifierScreen.h"
#include "screens/LoginPasswordScreen.h"
#include "screens/TwoFactorScreen.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_auth(new AuthIntegration(this))
    , m_personal(new PersonalIntegration(this))
    , m_stack(new QStackedWidget(this))
{
    setWindowTitle(tr("MyQtApp"));
    resize(480, 520);

    m_identifierScreen = new LoginIdentifierScreen(m_auth, &m_authRequest, m_stack);
    m_passwordScreen = new LoginPasswordScreen(m_auth, &m_authRequest, m_stack);
    m_twoFactorScreen = new TwoFactorScreen(m_auth, &m_authRequest, m_stack);
    m_homeScreen = new HomeScreen(m_auth, m_stack);

    m_stack->addWidget(m_identifierScreen);
    m_stack->addWidget(m_passwordScreen);
    m_stack->addWidget(m_twoFactorScreen);
    m_stack->addWidget(m_homeScreen);
    setCentralWidget(m_stack);

    connect(m_identifierScreen, &LoginIdentifierScreen::identified, this, &MainWindow::showPassword);
    connect(m_passwordScreen, &LoginPasswordScreen::back, this, &MainWindow::showIdentifier);
    connect(m_passwordScreen, &LoginPasswordScreen::loggedIn, this, &MainWindow::showTwoFactor);
    connect(m_twoFactorScreen, &TwoFactorScreen::verified, this, &MainWindow::showHome);
    connect(m_homeScreen, &HomeScreen::logout, this, [this]() {
        m_homeScreen->resetSession();
        SecuritySession::instance().clear(); // apaga chave privada e chave local da memória
        m_session = TwoFactorResponse();
        m_auth->setBearerToken(QString());
        m_personal->setBearerToken(QString());
        m_authRequest.clear();
        showIdentifier();
    });

    showIdentifier();
}

void MainWindow::showIdentifier()
{
    m_stack->setCurrentWidget(m_identifierScreen);
    m_identifierScreen->reset();
}

void MainWindow::showPassword()
{
    m_stack->setCurrentWidget(m_passwordScreen);
    m_passwordScreen->reset();
}

void MainWindow::showTwoFactor()
{
    m_stack->setCurrentWidget(m_twoFactorScreen);
    m_twoFactorScreen->reset();
}

void MainWindow::showHome(const TwoFactorResponse &session)
{
    m_session = session;
    m_auth->setBearerToken(session.accessToken);
    m_personal->setBearerToken(session.accessToken);

    // Gera o par RSA-8192 em segundo plano e deriva a chave local (AES) da senha.
    // Depois disso a senha não fica mais guardada.
    SecuritySession::instance().begin(m_authRequest.login(), m_authRequest.password());
    m_authRequest.clearPassword();

    m_homeScreen->setUser(session.user);
    m_stack->setCurrentWidget(m_homeScreen);

    if (width() < 1100 || height() < 700)
        resize(1200, 760);

    Popup::success(this, tr("Login realizado com sucesso."));
}
