#include "ContentPage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "components/labels/TextLabel.h"

ContentPage::ContentPage(const QString &title, const QString &subtitle, QWidget *parent)
    : QScrollArea(parent)
    , m_column(new QVBoxLayout())
{
    setObjectName("ContentPage");
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *columnWidget = new QWidget();
    columnWidget->setObjectName("ContentPageBody");
    columnWidget->setMaximumWidth(MaxContentWidth);
    columnWidget->setLayout(m_column);
    m_column->setContentsMargins(0, 0, 0, 0);
    m_column->setSpacing(16);

    m_column->addWidget(new TextLabel(title, TextLabel::Variant::Title, columnWidget));
    if (!subtitle.isEmpty())
        m_column->addWidget(new TextLabel(subtitle, TextLabel::Variant::Subtitle, columnWidget));
    m_column->addStretch();

    auto *body = new QWidget();
    body->setObjectName("ContentPageBody");
    auto *layout = new QHBoxLayout(body);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->addWidget(columnWidget, 1);
    layout->addStretch(0);

    setWidget(body);
}

void ContentPage::addContent(QWidget *widget)
{
    // antes do stretch final
    m_column->insertWidget(m_column->count() - 1, widget);
}
