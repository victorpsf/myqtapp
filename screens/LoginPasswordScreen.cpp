#include "LoginPasswordScreen.h"

#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include "components/fields/InputField.h"
#include "components/forms/FormCard.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "integration/auth/AuthIntegration.h"
#include "models/requests/AuthenticationRequest.h"

LoginPasswordScreen::LoginPasswordScreen(AuthIntegration *auth, AuthenticationRequest *request,
                                         QWidget *parent)
    : QWidget(parent)
    , m_auth(auth)
    , m_request(request)
    , m_card(new FormCard(tr("Digite sua senha")))
    , m_loginLabel(new TextLabel(QString(), TextLabel::Variant::Subtitle))
    , m_password(new InputField(tr("Senha")))
{
    m_password->setPasswordMode(true);

    m_card->addField(m_loginLabel);
    m_card->addField(m_password);

    QPushButton *backButton = m_card->addButton(tr("Voltar"));
    QPushButton *loginButton = m_card->addButton(tr("Logar"), true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(FormCard::centered(m_card, this));

    connect(backButton, &QPushButton::clicked, this, &LoginPasswordScreen::goBack);
    connect(loginButton, &QPushButton::clicked, this, &LoginPasswordScreen::submit);
    connect(m_password, &InputField::submitted, this, &LoginPasswordScreen::submit);
}

void LoginPasswordScreen::reset()
{
    const QString kind = m_request->isEmail() ? tr("E-mail") : tr("Usuário");
    m_loginLabel->setText(QStringLiteral("%1: <b>%2</b>").arg(kind, m_request->login().toHtmlEscaped()));
    m_password->clear();
    m_card->setBusy(false);
    m_password->focusInput();
}

void LoginPasswordScreen::goBack()
{
    m_request->clearPassword();
    m_password->clear();
    emit back();
}

void LoginPasswordScreen::submit()
{
    const QString password = m_password->text();
    if (password.isEmpty()) {
        Popup::warning(this, tr("Informe a senha."));
        m_password->focusInput();
        return;
    }

    m_request->setPassword(password);
    m_card->setBusy(true);

    QPointer<LoginPasswordScreen> self(this);
    m_auth->login(*m_request, [self](const IntegrationResponse<LoginResponse> &response) {
        if (!self)
            return;

        self->m_card->setBusy(false);

        if (response.hasError()) {
            Popup::error(self, response.error);
            self->m_password->focusInput();
            return;
        }

        if (response.data)
            self->m_request->setTwoFactorToken(response.data->twoFactorToken);

        emit self->loggedIn();
    });
}
