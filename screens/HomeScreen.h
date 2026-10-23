#pragma once

#include <QHash>
#include <QPointer>
#include <QWidget>

#include "models/entity/User.h"
#include "screens/ProfileWindow.h"

class AuthIntegration;
class QStackedWidget;
class SideMenu;
class TextLabel;
class TopBar;

// Tela principal após o login: menu superior fixo + menu lateral recolhível
class HomeScreen : public QWidget
{
    Q_OBJECT

public:
    // Abaixo desta largura o menu lateral recolhe automaticamente
    static constexpr int CompactBreakpoint = 900;

    explicit HomeScreen(AuthIntegration *auth, QWidget *parent = nullptr);

    void setUser(const User &user);
    void resetSession();

signals:
    void logout();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void toggleMenu();
    void updateResponsiveMenu();
    void openProfile();
    void showPage(const QString &id, const QString &groupText, const QString &itemText);

    QWidget *createWelcomePage();
    QWidget *createPage(const QString &id, const QString &groupText, const QString &itemText);
    QWidget *createPlaceholderPage(const QString &groupText, const QString &itemText);

    AuthIntegration *m_auth;
    TopBar *m_topBar;
    SideMenu *m_sideMenu;
    QStackedWidget *m_pages;
    QWidget *m_welcomePage = nullptr;
    TextLabel *m_welcomeTitle = nullptr;
    QHash<QString, QWidget *> m_pagesById;
    QPointer<ProfileWindow> m_profileWindow;

    bool m_narrow = false;
    bool m_userCollapsed = false; // preferência do usuário em telas largas
};
