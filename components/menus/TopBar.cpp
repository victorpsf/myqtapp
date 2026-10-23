#include "TopBar.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QToolButton>

#include "components/labels/AvatarLabel.h"

TopBar::TopBar(QWidget *parent)
    : QFrame(parent)
    , m_menuButton(new QToolButton(this))
    , m_title(new QLabel(this))
    , m_userButton(new QToolButton(this))
    , m_logoutButton(new QToolButton(this))
{
    setObjectName("TopBar");
    setFixedHeight(Height);

    m_menuButton->setObjectName("TopBarMenuButton");
    m_menuButton->setText(QStringLiteral("☰"));
    m_menuButton->setToolTip(tr("Mostrar/ocultar menu"));
    m_menuButton->setCursor(Qt::PointingHandCursor);

    m_title->setObjectName("TopBarTitle");

    m_userButton->setObjectName("TopBarUserButton");
    m_userButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_userButton->setIconSize(QSize(32, 32));
    m_userButton->setToolTip(tr("Ver perfil"));
    m_userButton->setCursor(Qt::PointingHandCursor);

    m_logoutButton->setObjectName("TopBarLogoutButton");
    m_logoutButton->setText(tr("Sair"));
    m_logoutButton->setCursor(Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 16, 0);
    layout->setSpacing(12);
    layout->addWidget(m_menuButton);
    layout->addWidget(m_title);
    layout->addStretch();
    layout->addWidget(m_userButton);
    layout->addWidget(m_logoutButton);

    connect(m_menuButton, &QToolButton::clicked, this, &TopBar::menuToggleRequested);
    connect(m_userButton, &QToolButton::clicked, this, &TopBar::userClicked);
    connect(m_logoutButton, &QToolButton::clicked, this, &TopBar::logoutRequested);
}

void TopBar::setTitle(const QString &title)
{
    m_title->setText(title);
}

void TopBar::setUser(const User &user)
{
    const QString name = displayName(user);
    m_userButton->setText(name);
    m_userButton->setIcon(QIcon(AvatarLabel::render(name, 32, devicePixelRatioF())));
}

QString TopBar::displayName(const User &user)
{
    if (!user.name.isEmpty())
        return user.name;
    if (!user.username.isEmpty())
        return user.username;
    return user.email.isEmpty() ? tr("Usuário") : user.email;
}
