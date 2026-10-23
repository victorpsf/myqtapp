#pragma once

#include <QFrame>
#include <QList>
#include <QString>

class QButtonGroup;
class QVBoxLayout;
class QVariantAnimation;
class SideMenuGroupHeader;

// Menu lateral com grupos expansíveis.
// Expandido: ícone + texto. Recolhido: só os ícones (clicar em um expande o menu).
class SideMenu : public QFrame
{
    Q_OBJECT

public:
    static constexpr int ExpandedWidth = 280;
    static constexpr int CollapsedWidth = 60;

    struct Item
    {
        QString id;
        QString text;
    };

    struct Group
    {
        QString icon;
        QString text;
        QList<Item> items;
    };

    explicit SideMenu(QWidget *parent = nullptr);

    void setGroups(const QList<Group> &groups);

    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed);
    void toggle() { setCollapsed(!m_collapsed); }

    void clearSelection();

signals:
    void itemSelected(const QString &id, const QString &groupText, const QString &itemText);
    void collapsedChanged(bool collapsed);

private:
    struct GroupWidgets
    {
        SideMenuGroupHeader *header = nullptr;
        QWidget *items = nullptr;
        bool open = false;
    };

    void setGroupOpen(int index, bool open);
    void applyContentState();

    QVBoxLayout *m_groupsLayout;
    QButtonGroup *m_itemButtons;
    QVariantAnimation *m_animation;
    QList<GroupWidgets> m_groups;
    bool m_collapsed = false;
};
