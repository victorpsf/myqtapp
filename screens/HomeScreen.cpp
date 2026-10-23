#include "HomeScreen.h"

#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "components/labels/TextLabel.h"
#include "components/menus/SideMenu.h"
#include "components/menus/TopBar.h"
#include "screens/ProfileWindow.h"
#include "screens/keys/AsymmetricDecryptionScreen.h"
#include "screens/keys/AsymmetricEncryptionScreen.h"
#include "screens/keys/AsymmetricKeyScreen.h"
#include "screens/keys/HashScreen.h"
#include "screens/keys/SymmetricDecryptionScreen.h"
#include "screens/keys/SymmetricEncryptionScreen.h"

HomeScreen::HomeScreen(AuthIntegration *auth, QWidget *parent)
    : QWidget(parent)
    , m_auth(auth)
    , m_topBar(new TopBar(this))
    , m_sideMenu(new SideMenu(this))
    , m_pages(new QStackedWidget(this))
{
    m_topBar->setTitle(tr("MyQtApp"));

    m_sideMenu->setGroups({
        {QStringLiteral("📋"), tr("Cadastrar"),
         {
             {QStringLiteral("cadastrar.despesas_fixas"), tr("Despesas fixas")},
             {QStringLiteral("cadastrar.renda"), tr("Renda")},
         }},
        {QStringLiteral("💳"), tr("Informar pagamento"),
         {
             {QStringLiteral("pagamento.credito"), tr("Registrar pagamento (crédito)")},
             {QStringLiteral("pagamento.debito"), tr("Registrar pagamento (débito)")},
             {QStringLiteral("pagamento.pix"), tr("Registrar pagamento (pix)")},
             {QStringLiteral("pagamento.consultar"), tr("Consultar")},
         }},
        {QStringLiteral("🔒"), tr("Segurança"),
         {
             {QStringLiteral("seguranca.criar_chaves"), tr("Criar chaves (assimétrica)")},
             {QStringLiteral("seguranca.criptografar_assimetrica"), tr("Criptografar (assimétrica)")},
             {QStringLiteral("seguranca.descriptografar_assimetrica"),
              tr("Descriptografar (assimétrica)")},
             {QStringLiteral("seguranca.criptografar_simetrica"), tr("Criptografar (simétrica)")},
             {QStringLiteral("seguranca.descriptografar_simetrica"),
              tr("Descriptografar (simétrica)")},
             {QStringLiteral("seguranca.hash"), tr("Criar hash")},
         }},
    });

    m_welcomePage = createWelcomePage();
    m_pages->addWidget(m_welcomePage);

    auto *body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    body->addWidget(m_sideMenu);
    body->addWidget(m_pages, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_topBar);
    layout->addLayout(body, 1);

    connect(m_topBar, &TopBar::menuToggleRequested, this, &HomeScreen::toggleMenu);
    connect(m_topBar, &TopBar::userClicked, this, &HomeScreen::openProfile);
    connect(m_topBar, &TopBar::logoutRequested, this, &HomeScreen::logout);
    connect(m_sideMenu, &SideMenu::itemSelected, this, &HomeScreen::showPage);
}

void HomeScreen::setUser(const User &user)
{
    m_topBar->setUser(user);
    m_welcomeTitle->setText(tr("Bem-vindo, %1").arg(TopBar::displayName(user)));
}

void HomeScreen::resetSession()
{
    if (m_profileWindow)
        m_profileWindow->close();

    m_sideMenu->clearSelection();
    m_pages->setCurrentWidget(m_welcomePage);

    // Descarta as páginas abertas (podem conter chaves e textos sensíveis)
    for (QWidget *page : std::as_const(m_pagesById)) {
        m_pages->removeWidget(page);
        page->deleteLater();
    }
    m_pagesById.clear();
}

void HomeScreen::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateResponsiveMenu();
}

void HomeScreen::toggleMenu()
{
    m_sideMenu->toggle();
    if (!m_narrow)
        m_userCollapsed = m_sideMenu->isCollapsed();
}

void HomeScreen::updateResponsiveMenu()
{
    const bool narrow = width() < CompactBreakpoint;
    if (narrow == m_narrow)
        return;

    m_narrow = narrow;
    m_sideMenu->setCollapsed(narrow || m_userCollapsed);
}

void HomeScreen::openProfile()
{
    if (m_profileWindow) {
        m_profileWindow->raise();
        m_profileWindow->activateWindow();
        return;
    }

    m_profileWindow = new ProfileWindow(m_auth, window());
    m_profileWindow->show();
}

void HomeScreen::showPage(const QString &id, const QString &groupText, const QString &itemText)
{
    QWidget *page = m_pagesById.value(id);
    if (!page) {
        page = createPage(id, groupText, itemText);
        m_pagesById.insert(id, page);
        m_pages->addWidget(page);
    }
    m_pages->setCurrentWidget(page);
}

QWidget *HomeScreen::createWelcomePage()
{
    auto *page = new QWidget(m_pages);
    m_welcomeTitle = new TextLabel(tr("Bem-vindo"), TextLabel::Variant::Title, page);

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(8);
    layout->addWidget(m_welcomeTitle);
    layout->addWidget(new TextLabel(tr("Selecione uma opção no menu lateral para começar."),
                                    TextLabel::Variant::Subtitle, page));
    layout->addStretch();
    return page;
}

QWidget *HomeScreen::createPage(const QString &id, const QString &groupText,
                               const QString &itemText)
{
    if (id == QLatin1String("seguranca.criar_chaves"))
        return new AsymmetricKeyScreen(m_pages);
    if (id == QLatin1String("seguranca.criptografar_assimetrica"))
        return new AsymmetricEncryptionScreen(m_pages);
    if (id == QLatin1String("seguranca.descriptografar_assimetrica"))
        return new AsymmetricDecryptionScreen(m_pages);
    if (id == QLatin1String("seguranca.criptografar_simetrica"))
        return new SymmetricEncryptionScreen(m_pages);
    if (id == QLatin1String("seguranca.descriptografar_simetrica"))
        return new SymmetricDecryptionScreen(m_pages);
    if (id == QLatin1String("seguranca.hash"))
        return new HashScreen(m_pages);

    return createPlaceholderPage(groupText, itemText);
}

QWidget *HomeScreen::createPlaceholderPage(const QString &groupText, const QString &itemText)
{
    auto *page = new QWidget(m_pages);

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(8);
    layout->addWidget(new TextLabel(QStringLiteral("%1 › %2").arg(groupText, itemText),
                                    TextLabel::Variant::Caption, page));
    layout->addWidget(new TextLabel(itemText, TextLabel::Variant::Title, page));
    layout->addWidget(new TextLabel(tr("Tela em construção."), TextLabel::Variant::Subtitle, page));
    layout->addStretch();
    return page;
}
