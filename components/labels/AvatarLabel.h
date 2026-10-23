#pragma once

#include <QLabel>
#include <QPixmap>

// Círculo colorido com as iniciais do nome
class AvatarLabel : public QLabel
{
    Q_OBJECT

public:
    explicit AvatarLabel(int size = 40, QWidget *parent = nullptr);

    void setName(const QString &name);

    static QPixmap render(const QString &name, int size, qreal devicePixelRatio = 1.0);
    static QString initials(const QString &name);

private:
    int m_size;
};
