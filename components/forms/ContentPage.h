#pragma once

#include <QScrollArea>

class QVBoxLayout;

// Página rolável do conteúdo principal: título, subtítulo e seções
class ContentPage : public QScrollArea
{
    Q_OBJECT

public:
    static constexpr int MaxContentWidth = 760;

    ContentPage(const QString &title, const QString &subtitle, QWidget *parent = nullptr);

    // Adiciona ao final da coluna de conteúdo
    void addContent(QWidget *widget);

private:
    QVBoxLayout *m_column;
};
