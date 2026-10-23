#include "TwoFactorScreen.h"

#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include "components/fields/InputField.h"
#include "components/forms/FormCard.h"
#include "components/popups/Popup.h"
#include "integration/auth/AuthIntegration.h"
#include "models/requests/AuthenticationRequest.h"

TwoFactorScreen::TwoFactorScreen(AuthIntegration *auth, AuthenticationRequest *request,
                                 QWidget *parent)
    : QWidget(parent)
    , m_auth(auth)
    , m_request(request)
    , m_card(new FormCard(tr("Verificação em duas etapas"),
                          tr("Digite o código de verificação que enviamos para você.")))
    , m_code(new InputField(tr("Código 2FA")))
{
    m_code->setDigitsOnly(CodeLength);
    m_code->setCodeStyle();
    m_code->setPlaceholder(QString(CodeLength, '0'));
    m_card->addField(m_code);

    QPushButton *resendButton = m_card->addButton(tr("Reenviar código"));
    QPushButton *verifyButton = m_card->addButton(tr("Verificar"), true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(FormCard::centered(m_card, this));

    connect(resendButton, &QPushButton::clicked, this, &TwoFactorScreen::resend);
    connect(verifyButton, &QPushButton::clicked, this, &TwoFactorScreen::verify);
    connect(m_code, &InputField::submitted, this, &TwoFactorScreen::verify);
}

void TwoFactorScreen::reset()
{
    m_code->clear();
    m_card->setBusy(false);
    m_code->focusInput();
}

void TwoFactorScreen::verify()
{
    const QString code = m_code->text().trimmed();
    if (code.size() != CodeLength) {
        Popup::warning(this, tr("Informe o código de %1 dígitos.").arg(CodeLength));
        m_code->focusInput();
        return;
    }

    m_request->setTwoFactorCode(code);
    m_card->setBusy(true);

    QPointer<TwoFactorScreen> self(this);
    m_auth->verifyTwoFactor(*m_request, [self](const IntegrationResponse<TwoFactorResponse> &response) {
        if (!self)
            return;

        self->m_card->setBusy(false);

        if (response.hasError()) {
            Popup::error(self, response.error);
            self->m_code->focusInput();
            return;
        }

        emit self->verified(response.data.value_or(TwoFactorResponse()));
    });
}

void TwoFactorScreen::resend()
{
    m_card->setBusy(true);

    QPointer<TwoFactorScreen> self(this);
    m_auth->resendTwoFactor(*m_request, [self](const IntegrationResponse<ResendCodeResponse> &response) {
        if (!self)
            return;

        self->m_card->setBusy(false);

        if (response.hasError()) {
            Popup::error(self, response.error);
            return;
        }

        const QString destination = response.data ? response.data->destination : QString();
        Popup::info(self, destination.isEmpty()
                              ? tr("Um novo código foi enviado.")
                              : tr("Um novo código foi enviado para %1.").arg(destination));
        self->m_code->clear();
        self->m_code->focusInput();
    });
}
