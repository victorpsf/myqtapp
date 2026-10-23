#pragma once

#include <QDialog>

#include "models/entity/UserProfile.h"

class AuthIntegration;
class AvatarLabel;
class QFormLayout;
class QStackedWidget;
class TextLabel;

// Janela de perfil. Abre em loading e segue o fluxo:
//   1) aguarda a chave RSA do cliente (gerada no login)
//   2) handshake com o auth (envia a pública do cliente, recebe a do servidor)
//   3) busca o perfil criptografado e descriptografa
class ProfileWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileWindow(AuthIntegration *auth, QWidget *parent = nullptr);

private:
    enum Page { LoadingPage, ProfilePage, ErrorPage };

    void load();
    void waitForKeys();
    void performHandshake();
    void fetchProfile();

    void showLoading(const QString &status);
    void showProfile(const UserProfile &profile);
    void showError(const QString &message);
    void stopWaitingForKeys();

    QWidget *createLoadingPage();
    QWidget *createProfilePage();
    QWidget *createErrorPage();

    AuthIntegration *m_auth;
    QStackedWidget *m_pages;

    TextLabel *m_status = nullptr;
    AvatarLabel *m_avatar = nullptr;
    TextLabel *m_name = nullptr;
    TextLabel *m_email = nullptr;
    QFormLayout *m_details = nullptr;
    TextLabel *m_errorMessage = nullptr;

    QMetaObject::Connection m_keysReadyConnection;
    QMetaObject::Connection m_keysFailedConnection;
};
