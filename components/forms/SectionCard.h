#pragma once

#include <QFrame>

class QLayout;
class QVBoxLayout;

// Bloco branco com título opcional, usado nas páginas de conteúdo
class SectionCard : public QFrame
{
    Q_OBJECT

public:
    explicit SectionCard(const QString &title = QString(), QWidget *parent = nullptr);

    void addWidget(QWidget *widget);
    void addLayout(QLayout *layout);

    QVBoxLayout *body() const { return m_body; }

private:
    QVBoxLayout *m_body;
};
