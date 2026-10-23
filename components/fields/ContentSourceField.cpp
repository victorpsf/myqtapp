#include "ContentSourceField.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QVBoxLayout>

#include "components/fields/FilePickerField.h"
#include "components/fields/TextAreaField.h"
#include "components/labels/TextLabel.h"

ContentSourceField::ContentSourceField(const QString &prompt, const QString &textLabel,
                                       const QString &fileLabel, QWidget *parent)
    : QWidget(parent)
    , m_textMode(new QRadioButton(tr("Texto"), this))
    , m_fileMode(new QRadioButton(tr("Arquivo"), this))
    , m_text(new TextAreaField(textLabel, this))
    , m_file(new FilePickerField(fileLabel, this))
{
    auto *modes = new QButtonGroup(this);
    modes->addButton(m_textMode);
    modes->addButton(m_fileMode);
    m_textMode->setChecked(true);

    auto *modeRow = new QHBoxLayout();
    modeRow->setSpacing(16);
    modeRow->addWidget(new TextLabel(prompt, TextLabel::Variant::Caption, this));
    modeRow->addWidget(m_textMode);
    modeRow->addWidget(m_fileMode);
    modeRow->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addLayout(modeRow);
    layout->addWidget(m_text);
    layout->addWidget(m_file);

    connect(m_fileMode, &QRadioButton::toggled, this, &ContentSourceField::updateMode);
    connect(m_text->editor(), &QPlainTextEdit::textChanged, this, &ContentSourceField::changed);
    connect(m_file, &FilePickerField::fileSelected, this, &ContentSourceField::changed);

    updateMode();
}

bool ContentSourceField::isFileMode() const
{
    return m_fileMode->isChecked();
}

QString ContentSourceField::text() const
{
    return m_text->text();
}

QString ContentSourceField::filePath() const
{
    return m_file->filePath();
}

void ContentSourceField::setPlaceholder(const QString &placeholder)
{
    m_text->setPlaceholder(placeholder);
}

void ContentSourceField::setMonospace(bool monospace)
{
    m_text->setMonospace(monospace);
}

void ContentSourceField::updateMode()
{
    m_text->setVisible(!isFileMode());
    m_file->setVisible(isFileMode());
    emit changed();
}
