#pragma once

#include <QFrame>

#include "models/entity/User.h"

class QLabel;
class QToolButton;

// Barra superior de altura fixa: botão do menu lateral, título e usuário
class TopBar : public QFrame
{
    Q_OBJECT

public:
    static constexpr int Height = 56;

    explicit TopBar(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setUser(const User &user);

    static QString displayName(const User &user);

signals:
    void menuToggleRequested();
    void userClicked();
    void logoutRequested();

private:
    QToolButton *m_menuButton;
    QLabel *m_title;
    QToolButton *m_userButton;
    QToolButton *m_logoutButton;
};
