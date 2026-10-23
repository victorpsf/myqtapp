#include "ProgressStatus.h"

#include <QHBoxLayout>
#include <QProgressBar>

#include "components/labels/TextLabel.h"

ProgressStatus::ProgressStatus(QWidget *parent)
    : QWidget(parent)
    , m_bar(new QProgressBar(this))
    , m_message(new TextLabel(QString(), TextLabel::Variant::Caption, this))
{
    m_bar->setRange(0, 0);
    m_bar->setTextVisible(false);
    m_bar->setFixedSize(140, 6);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(m_bar);
    layout->addWidget(m_message, 1);

    hide();
}

void ProgressStatus::start(const QString &message)
{
    m_message->setText(message);
    show();
}

void ProgressStatus::stop()
{
    hide();
}
