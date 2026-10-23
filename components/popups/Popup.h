#pragma once

#include <QColor>
#include <QDialog>
#include <QString>

enum class PopupType { Info, Success, Warning, Error };

// Aparência de cada tipo — pode ser trocada com Popup::setStyle()
struct PopupStyle
{
    QString icon;
    QString defaultTitle;
    QColor accent;
    QColor background;
};

struct PopupConfig
{
    PopupType type = PopupType::Info;
    QString title;                    // vazio = título padrão do tipo
    QString message;
    QString confirmText = "OK";
    QString cancelText;               // vazio = sem botão de cancelar
};

class Popup : public QDialog
{
    Q_OBJECT

public:
    explicit Popup(const PopupConfig &config, QWidget *parent = nullptr);

    // Retorna true se o usuário confirmou
    static bool display(QWidget *parent, const PopupConfig &config);

    static void info(QWidget *parent, const QString &message, const QString &title = QString());
    static void success(QWidget *parent, const QString &message, const QString &title = QString());
    static void warning(QWidget *parent, const QString &message, const QString &title = QString());
    static void error(QWidget *parent, const QString &message, const QString &title = QString());

    static PopupStyle style(PopupType type);
    static void setStyle(PopupType type, const PopupStyle &style);
};
