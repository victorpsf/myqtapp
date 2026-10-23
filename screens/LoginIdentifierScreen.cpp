#include "LoginIdentifierScreen.h"

#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include "components/fields/InputField.h"
#include "components/forms/FormCard.h"
#include "components/popups/Popup.h"
#include "integration/auth/AuthIntegration.h"
#include "models/requests/AuthenticationRequest.h"

LoginIdentifierScreen::LoginIdentifierScreen(AuthIntegration *auth, AuthenticationRequest *request,
                                             QWidget *parent)
    : QWidget(parent)
    , m_auth(auth)
    , m_request(request)
    , m_card(new FormCard(tr("Entrar"), tr("Informe seu usuário ou e-mail para continuar.")))
    , m_login(new InputField(tr("Usuário ou e-mail")))
{
    m_login->setPlaceholder(tr("usuario ou nome@email.com"));
    m_card->addField(m_login);

    QPushButton *continueButton = m_card->addButton(tr("Continuar"), true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(FormCard::centered(m_card, this));

    connect(continueButton, &QPushButton::clicked, this, &LoginIdentifierScreen::submit);
    connect(m_login, &InputField::submitted, this, &LoginIdentifierScreen::submit);
}

void LoginIdentifierScreen::reset()
{
    m_login->setText(m_request->login());
    m_card->setBusy(false);
    m_login->focusInput();
}

void LoginIdentifierScreen::submit()
{
    const QString login = m_login->text().trimmed();
    if (login.isEmpty()) {
        Popup::warning(this, tr("Informe o usuário ou e-mail."));
        m_login->focusInput();
        return;
    }

    m_request->setLogin(login);
    m_card->setBusy(true);

    QPointer<LoginIdentifierScreen> self(this);
    m_auth->identify(*m_request, [self](const IntegrationResponse<IdentifyResponse> &response) {
        if (!self)
            return;

        self->m_card->setBusy(false);

        if (response.hasError()) {
            Popup::error(self, response.error);
            return;
        }

        if (response.data && !response.data->exists) {
            Popup::error(self, tr("Usuário ou e-mail não encontrado."));
            return;
        }

        emit self->identified();
    });
}
