#include "AvatarLabel.h"

#include <QColor>
#include <QPainter>
#include <QStringList>

namespace {

QColor colorFor(const QString &name)
{
    static const QList<QColor> palette = {
        QColor("#2563eb"), QColor("#7c3aed"), QColor("#db2777"),
        QColor("#ea580c"), QColor("#16a34a"), QColor("#0891b2"),
    };
    return palette.at(int(qHash(name) % uint(palette.size())));
}

} // namespace

AvatarLabel::AvatarLabel(int size, QWidget *parent)
    : QLabel(parent)
    , m_size(size)
{
    setFixedSize(size, size);
}

void AvatarLabel::setName(const QString &name)
{
    setPixmap(render(name, m_size, devicePixelRatioF()));
    setToolTip(name);
}

QString AvatarLabel::initials(const QString &name)
{
    const QStringList parts = name.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return QStringLiteral("?");
    if (parts.size() == 1)
        return parts.first().left(2).toUpper();
    return (parts.first().left(1) + parts.last().left(1)).toUpper();
}

QPixmap AvatarLabel::render(const QString &name, int size, qreal devicePixelRatio)
{
    QPixmap pixmap(int(size * devicePixelRatio), int(size * devicePixelRatio));
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(colorFor(name));
    painter.drawEllipse(QRectF(0, 0, size, size));

    QFont font = painter.font();
    font.setPixelSize(qMax(10, size * 2 / 5));
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, initials(name));

    return pixmap;
}
