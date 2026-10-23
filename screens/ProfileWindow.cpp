#include "ProfileWindow.h"

#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "components/labels/AvatarLabel.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "integration/auth/AuthIntegration.h"
#include "security/SecuritySession.h"

ProfileWindow::ProfileWindow(AuthIntegration *auth, QWidget *parent)
    : QDialog(parent)
    , m_auth(auth)
    , m_pages(new QStackedWidget(this))
{
    setObjectName("ProfileWindow");
    setWindowTitle(tr("Meu perfil"));
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(420, 460);

    m_pages->insertWidget(LoadingPage, createLoadingPage());
    m_pages->insertWidget(ProfilePage, createProfilePage());
    m_pages->insertWidget(ErrorPage, createErrorPage());

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pages);

    load();
}

QWidget *ProfileWindow::createLoadingPage()
{
    auto *page = new QWidget(this);

    auto *progress = new QProgressBar(page);
    progress->setRange(0, 0); // indeterminado
    progress->setTextVisible(false);
    progress->setFixedSize(220, 6);

    m_status = new TextLabel(QString(), TextLabel::Variant::Subtitle, page);
    m_status->setAlignment(Qt::AlignCenter);

    auto *hint = new TextLabel(
        tr("As informações do perfil trafegam criptografadas. Na primeira abertura após o login "
           "a geração da chave RSA de %1 bits pode levar alguns segundos.")
            .arg(RsaKey::DefaultBits),
        TextLabel::Variant::Caption, page);
    hint->setAlignment(Qt::AlignCenter);

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);
    layout->addStretch();
    layout->addWidget(progress, 0, Qt::AlignHCenter);
    layout->addWidget(m_status);
    layout->addWidget(hint);
    layout->addStretch();
    return page;
}

QWidget *ProfileWindow::createProfilePage()
{
    auto *page = new QWidget(this);

    m_avatar = new AvatarLabel(72, page);
    m_name = new TextLabel(QString(), TextLabel::Variant::Title, page);
    m_name->setAlignment(Qt::AlignCenter);
    m_email = new TextLabel(QString(), TextLabel::Variant::Subtitle, page);
    m_email->setAlignment(Qt::AlignCenter);

    m_details = new QFormLayout();
    m_details->setHorizontalSpacing(16);
    m_details->setVerticalSpacing(10);
    m_details->setLabelAlignment(Qt::AlignRight);

    auto *secure = new TextLabel(tr("🔒 Dados recebidos criptografados (RSA-%1 + AES-256-CBC)")
                                     .arg(RsaKey::DefaultBits),
                                 TextLabel::Variant::Caption, page);
    secure->setAlignment(Qt::AlignCenter);

    auto *close = new QPushButton(tr("Fechar"), page);
    connect(close, &QPushButton::clicked, this, &QDialog::close);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(close);

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 28, 32, 20);
    layout->setSpacing(12);
    layout->addWidget(m_avatar, 0, Qt::AlignHCenter);
    layout->addWidget(m_name);
    layout->addWidget(m_email);
    layout->addSpacing(8);
    layout->addLayout(m_details);
    layout->addStretch();
    layout->addWidget(secure);
    layout->addLayout(buttons);
    return page;
}

QWidget *ProfileWindow::createErrorPage()
{
    auto *page = new QWidget(this);

    auto *title = new TextLabel(tr("Não foi possível carregar o perfil"), TextLabel::Variant::Title,
                                page);
    title->setAlignment(Qt::AlignCenter);
    m_errorMessage = new TextLabel(QString(), TextLabel::Variant::Subtitle, page);
    m_errorMessage->setAlignment(Qt::AlignCenter);

    auto *close = new QPushButton(tr("Fechar"), page);
    auto *retry = new QPushButton(tr("Tentar novamente"), page);
    retry->setProperty("primary", true);
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    connect(retry, &QPushButton::clicked, this, &ProfileWindow::load);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(close);
    buttons->addWidget(retry);
    buttons->addStretch();

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(12);
    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(m_errorMessage);
    layout->addSpacing(8);
    layout->addLayout(buttons);
    layout->addStretch();
    return page;
}

void ProfileWindow::load()
{
    waitForKeys();
}

void ProfileWindow::waitForKeys()
{
    SecuritySession &session = SecuritySession::instance();

    switch (session.state()) {
    case SecuritySession::State::Ready:
        performHandshake();
        return;

    case SecuritySession::State::Generating:
        showLoading(tr("Gerando chave de segurança RSA de %1 bits…").arg(RsaKey::DefaultBits));
        stopWaitingForKeys();
        m_keysReadyConnection = connect(&session, &SecuritySession::keysReady, this, [this]() {
            stopWaitingForKeys();
            performHandshake();
        });
        m_keysFailedConnection = connect(&session, &SecuritySession::keysFailed, this,
                                         [this](const QString &error) {
                                             stopWaitingForKeys();
                                             showError(error);
                                         });
        return;

    case SecuritySession::State::Failed:
        showError(session.errorString() + ' ' + tr("Faça login novamente."));
        return;

    case SecuritySession::State::Idle:
        showError(tr("Sessão de segurança não iniciada. Faça login novamente."));
        return;
    }
}

void ProfileWindow::performHandshake()
{
    SecuritySession &session = SecuritySession::instance();
    if (session.hasServerKey(AuthIntegration::ServiceName)) {
        fetchProfile();
        return;
    }

    showLoading(tr("Realizando handshake com o servidor…"));

    HandshakeRequest request;
    request.clientPublicKey = QString::fromUtf8(session.clientKey().publicPem());

    QPointer<ProfileWindow> self(this);
    m_auth->handshake(request, [self](const IntegrationResponse<HandshakeResponse> &response) {
        // guarda a chave do servidor mesmo se a janela já foi fechada
        const bool stored = response.isSuccess() && response.data
                            && SecuritySession::instance().setServerKey(
                                AuthIntegration::ServiceName,
                                response.data->serverPublicKey.toUtf8());
        if (!self)
            return;

        if (response.hasError()) {
            self->showError(response.error);
            return;
        }
        if (!stored) {
            self->showError(tr("Chave pública do servidor inválida."));
            return;
        }

        self->fetchProfile();
    });
}

void ProfileWindow::fetchProfile()
{
    showLoading(tr("Buscando informações do usuário…"));

    QPointer<ProfileWindow> self(this);
    m_auth->getProfile([self](const IntegrationResponse<EncryptedPayload> &response) {
        if (!self)
            return;

        if (response.hasError()) {
            self->showError(response.error);
            return;
        }
        if (!response.data) {
            self->showError(tr("O servidor não retornou dados."));
            return;
        }

        const std::optional<QByteArray> plain =
            SecuritySession::instance().openFrom(AuthIntegration::ServiceName, *response.data);
        if (!plain) {
            self->showError(tr("Não foi possível validar ou descriptografar a resposta do servidor."));
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(*plain);
        if (!document.isObject()) {
            self->showError(tr("Resposta do perfil em formato inválido."));
            return;
        }

        self->showProfile(UserProfile::fromJson(document.object()));
    });
}

void ProfileWindow::showLoading(const QString &status)
{
    m_status->setText(status);
    m_pages->setCurrentIndex(LoadingPage);
}

void ProfileWindow::showProfile(const UserProfile &profile)
{
    const QString name = profile.name.isEmpty() ? profile.username : profile.name;
    m_avatar->setName(name);
    m_name->setText(name);
    m_email->setText(profile.email);

    while (m_details->rowCount() > 0)
        m_details->removeRow(0);

    const QDateTime createdAt = QDateTime::fromString(profile.createdAt, Qt::ISODate);
    const QList<QPair<QString, QString>> rows = {
        {tr("Usuário"), profile.username},
        {tr("E-mail"), profile.email},
        {tr("Telefone"), profile.phone},
        {tr("Documento"), profile.document},
        {tr("Cargo"), profile.role},
        {tr("Empresa"), profile.company},
        {tr("Membro desde"), createdAt.isValid() ? createdAt.toString("dd/MM/yyyy")
                                                 : profile.createdAt},
    };

    for (const auto &row : rows) {
        if (row.second.isEmpty())
            continue;
        auto *label = new TextLabel(row.first, TextLabel::Variant::Caption);
        auto *value = new TextLabel(row.second, TextLabel::Variant::Body);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_details->addRow(label, value);
    }

    m_pages->setCurrentIndex(ProfilePage);
}

void ProfileWindow::showError(const QString &message)
{
    m_errorMessage->setText(message);
    m_pages->setCurrentIndex(ErrorPage);
    Popup::error(this, message);
}

void ProfileWindow::stopWaitingForKeys()
{
    disconnect(m_keysReadyConnection);
    disconnect(m_keysFailedConnection);
}
