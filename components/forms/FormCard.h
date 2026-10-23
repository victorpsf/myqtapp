#pragma once

#include <QFrame>
#include <QList>

class QHBoxLayout;
class QPushButton;
class QVBoxLayout;
class TextLabel;

// Cartão centralizado usado pelas telas de formulário:
// título, subtítulo, campos e linha de botões.
class FormCard : public QFrame
{
    Q_OBJECT

public:
    explicit FormCard(const QString &title, const QString &subtitle = QString(),
                      QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setSubtitle(const QString &subtitle);

    void addField(QWidget *field);
    QPushButton *addButton(const QString &text, bool primary = false);

    // Desabilita os botões enquanto uma requisição está em andamento
    void setBusy(bool busy);

    // Envolve o cartão em um widget que o centraliza na tela
    static QWidget *centered(FormCard *card, QWidget *parent);

private:
    TextLabel *m_title;
    TextLabel *m_subtitle;
    QVBoxLayout *m_fields;
    QHBoxLayout *m_buttons;
    QList<QPushButton *> m_buttonList;
};
