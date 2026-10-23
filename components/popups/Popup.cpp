#include "Popup.h"

#include <QHBoxLayout>
#include <QMap>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QMap<PopupType, PopupStyle> &styles()
{
    static QMap<PopupType, PopupStyle> map = {
        {PopupType::Info,    {QStringLiteral("ℹ"), QObject::tr("Informação"), QColor("#2563eb"), QColor("#eff6ff")}},
        {PopupType::Success, {QStringLiteral("✔"), QObject::tr("Sucesso"),    QColor("#16a34a"), QColor("#f0fdf4")}},
        {PopupType::Warning, {QStringLiteral("⚠"), QObject::tr("Atenção"),    QColor("#d97706"), QColor("#fffbeb")}},
        {PopupType::Error,   {QStringLiteral("✖"), QObject::tr("Erro"),       QColor("#dc2626"), QColor("#fef2f2")}},
    };
    return map;
}

} // namespace

Popup::Popup(const PopupConfig &config, QWidget *parent)
    : QDialog(parent)
{
    const PopupStyle popupStyle = style(config.type);
    const QString title = config.title.isEmpty() ? popupStyle.defaultTitle : config.title;
    const QString accent = popupStyle.accent.name();

    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(360);
    setStyleSheet(QStringLiteral(
        "QDialog { background: %1; border-top: 4px solid %2; }"
        "QLabel#PopupIcon { color: %2; font-size: 28px; }"
        "QLabel#PopupTitle { color: %2; font-size: 16px; font-weight: 600; }"
        "QLabel#PopupMessage { color: #111827; font-size: 13px; }"
        "QPushButton#PopupConfirm { background: %2; border-color: %2; color: #ffffff; }")
        .arg(popupStyle.background.name(), accent));

    auto *icon = new QLabel(popupStyle.icon, this);
    icon->setObjectName("PopupIcon");
    icon->setAlignment(Qt::AlignTop);

    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("PopupTitle");

    auto *message = new QLabel(config.message, this);
    message->setObjectName("PopupMessage");
    message->setWordWrap(true);
    message->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *texts = new QVBoxLayout();
    texts->setSpacing(6);
    texts->addWidget(titleLabel);
    texts->addWidget(message);

    auto *content = new QHBoxLayout();
    content->setSpacing(14);
    content->addWidget(icon);
    content->addLayout(texts, 1);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();

    if (!config.cancelText.isEmpty()) {
        auto *cancel = new QPushButton(config.cancelText, this);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        buttons->addWidget(cancel);
    }

    auto *confirm = new QPushButton(config.confirmText, this);
    confirm->setObjectName("PopupConfirm");
    confirm->setDefault(true);
    connect(confirm, &QPushButton::clicked, this, &QDialog::accept);
    buttons->addWidget(confirm);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 16);
    layout->setSpacing(18);
    layout->addLayout(content);
    layout->addLayout(buttons);
}

bool Popup::display(QWidget *parent, const PopupConfig &config)
{
    Popup popup(config, parent);
    return popup.exec() == QDialog::Accepted;
}

void Popup::info(QWidget *parent, const QString &message, const QString &title)
{
    display(parent, {PopupType::Info, title, message});
}

void Popup::success(QWidget *parent, const QString &message, const QString &title)
{
    display(parent, {PopupType::Success, title, message});
}

void Popup::warning(QWidget *parent, const QString &message, const QString &title)
{
    display(parent, {PopupType::Warning, title, message});
}

void Popup::error(QWidget *parent, const QString &message, const QString &title)
{
    display(parent, {PopupType::Error, title, message});
}

PopupStyle Popup::style(PopupType type)
{
    return styles().value(type);
}

void Popup::setStyle(PopupType type, const PopupStyle &style)
{
    styles().insert(type, style);
}
