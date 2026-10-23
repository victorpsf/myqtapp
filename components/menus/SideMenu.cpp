#include "SideMenu.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QVariantAnimation>

// Cabeçalho do grupo: [ícone] [texto ......] [seta]
class SideMenuGroupHeader : public QPushButton
{
public:
    SideMenuGroupHeader(const QString &icon, const QString &text, QWidget *parent)
        : QPushButton(parent)
        , m_icon(new QLabel(icon, this))
        , m_text(new QLabel(text, this))
        , m_arrow(new QLabel(this))
    {
        setObjectName("SideMenuGroup");
        setCursor(Qt::PointingHandCursor);
        setToolTip(text);
        setMinimumHeight(44);
        setFlat(true);

        m_icon->setObjectName("SideMenuIcon");
        m_icon->setFixedWidth(22);
        m_icon->setAlignment(Qt::AlignCenter);
        m_text->setObjectName("SideMenuText");
        m_arrow->setObjectName("SideMenuArrow");

        for (QLabel *label : {m_icon, m_text, m_arrow})
            label->setAttribute(Qt::WA_TransparentForMouseEvents);

        // margem esquerda fixa: o ícone fica centralizado quando o menu recolhe
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins((SideMenu::CollapsedWidth - 22) / 2, 0, 14, 0);
        layout->setSpacing(12);
        layout->addWidget(m_icon);
        layout->addWidget(m_text, 1);
        layout->addWidget(m_arrow);

        setOpen(false);
    }

    void setOpen(bool open) { m_arrow->setText(open ? QStringLiteral("▾") : QStringLiteral("▸")); }

    void setCompact(bool compact)
    {
        m_text->setVisible(!compact);
        m_arrow->setVisible(!compact);
    }

private:
    QLabel *m_icon;
    QLabel *m_text;
    QLabel *m_arrow;
};

SideMenu::SideMenu(QWidget *parent)
    : QFrame(parent)
    , m_groupsLayout(new QVBoxLayout())
    , m_itemButtons(new QButtonGroup(this))
    , m_animation(new QVariantAnimation(this))
{
    setObjectName("SideMenu");
    setFixedWidth(ExpandedWidth);

    m_itemButtons->setExclusive(true);

    auto *content = new QWidget();
    content->setObjectName("SideMenuContent");
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 12, 0, 12);
    contentLayout->setSpacing(0);
    m_groupsLayout->setSpacing(2);
    contentLayout->addLayout(m_groupsLayout);
    contentLayout->addStretch();

    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("SideMenuScroll");
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    m_animation->setDuration(180);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_animation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) { setFixedWidth(value.toInt()); });
    // ao expandir, os textos só aparecem no fim da animação (evita texto espremido)
    connect(m_animation, &QVariantAnimation::finished, this, &SideMenu::applyContentState);
}

void SideMenu::setGroups(const QList<Group> &groups)
{
    for (const GroupWidgets &group : std::as_const(m_groups)) {
        delete group.header;
        delete group.items;
    }
    m_groups.clear();

    for (const Group &group : groups) {
        const int index = int(m_groups.size());

        auto *header = new SideMenuGroupHeader(group.icon, group.text, this);

        auto *items = new QWidget(this);
        auto *itemsLayout = new QVBoxLayout(items);
        itemsLayout->setContentsMargins(0, 0, 0, 6);
        itemsLayout->setSpacing(2);

        for (const Item &item : group.items) {
            auto *button = new QPushButton(item.text, items);
            button->setObjectName("SideMenuItem");
            button->setCheckable(true);
            button->setCursor(Qt::PointingHandCursor);
            m_itemButtons->addButton(button);
            itemsLayout->addWidget(button);

            const QString groupText = group.text;
            connect(button, &QPushButton::clicked, this, [this, item, groupText]() {
                emit itemSelected(item.id, groupText, item.text);
            });
        }

        m_groupsLayout->addWidget(header);
        m_groupsLayout->addWidget(items);
        m_groups.append(GroupWidgets{header, items, false});

        connect(header, &QPushButton::clicked, this, [this, index]() {
            if (m_collapsed) {
                m_groups[index].open = true;
                setCollapsed(false);
                return;
            }
            setGroupOpen(index, !m_groups[index].open);
        });
    }

    applyContentState();
}

void SideMenu::setCollapsed(bool collapsed)
{
    if (collapsed == m_collapsed)
        return;

    m_collapsed = collapsed;

    if (collapsed)
        applyContentState(); // esconde textos antes de encolher

    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(collapsed ? CollapsedWidth : ExpandedWidth);
    m_animation->start();

    emit collapsedChanged(collapsed);
}

void SideMenu::clearSelection()
{
    m_itemButtons->setExclusive(false);
    for (QAbstractButton *button : m_itemButtons->buttons())
        button->setChecked(false);
    m_itemButtons->setExclusive(true);
}

void SideMenu::setGroupOpen(int index, bool open)
{
    GroupWidgets &group = m_groups[index];
    group.open = open;
    group.header->setOpen(open);
    group.items->setVisible(open && !m_collapsed);
}

void SideMenu::applyContentState()
{
    for (int i = 0; i < m_groups.size(); ++i) {
        m_groups[i].header->setCompact(m_collapsed);
        setGroupOpen(i, m_groups[i].open);
    }
}
