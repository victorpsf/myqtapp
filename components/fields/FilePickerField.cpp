#include "FilePickerField.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

#include "components/labels/TextLabel.h"

FilePickerField::FilePickerField(const QString &label, QWidget *parent)
    : QWidget(parent)
    , m_display(new QLineEdit(this))
    , m_button(new QPushButton(tr("Escolher arquivo…"), this))
{
    auto *title = new TextLabel(label, TextLabel::Variant::Caption, this);

    m_display->setReadOnly(true);
    m_display->setPlaceholderText(tr("Nenhum arquivo selecionado"));
    m_display->setMinimumHeight(38);
    m_button->setCursor(Qt::PointingHandCursor);
    m_button->setMinimumHeight(38);

    auto *row = new QHBoxLayout();
    row->setSpacing(8);
    row->addWidget(m_display, 1);
    row->addWidget(m_button);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(title);
    layout->addLayout(row);

    connect(m_button, &QPushButton::clicked, this, &FilePickerField::choose);
}

void FilePickerField::clear()
{
    m_path.clear();
    m_display->clear();
}

void FilePickerField::choose()
{
    const QString startDir = m_path.isEmpty() ? QDir::homePath() : QFileInfo(m_path).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, tr("Escolher arquivo"), startDir);
    if (path.isEmpty())
        return;

    m_path = path;
    const QFileInfo info(path);
    m_display->setText(QStringLiteral("%1  (%2)").arg(info.fileName(),
                                                      QLocale().formattedDataSize(info.size())));
    m_display->setToolTip(path);
    emit fileSelected(path);
}
