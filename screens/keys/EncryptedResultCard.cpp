#include "EncryptedResultCard.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPushButton>

#include "components/fields/TextAreaField.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "screens/keys/KeyToolsCommon.h"

EncryptedResultCard::EncryptedResultCard(QWidget *parent)
    : SectionCard(tr("Resultado"), parent)
    , m_summary(new TextLabel(QString(), TextLabel::Variant::Body))
    , m_output(new TextAreaField(tr("Conteúdo criptografado (base64)")))
    , m_copy(new QPushButton(tr("Copiar")))
    , m_save(new QPushButton(tr("Salvar em pasta…")))
{
    m_output->setReadOnly(true);
    m_output->setMonospace(true);

    m_copy->setCursor(Qt::PointingHandCursor);
    m_save->setProperty("primary", true);
    m_save->setCursor(Qt::PointingHandCursor);

    auto *actions = new QHBoxLayout();
    actions->addStretch();
    actions->addWidget(m_copy);
    actions->addWidget(m_save);

    addWidget(m_summary);
    addWidget(m_output);
    addLayout(actions);
    hide();

    connect(m_copy, &QPushButton::clicked, this,
            [this]() { QGuiApplication::clipboard()->setText(m_output->text()); });
    connect(m_save, &QPushButton::clicked, this, &EncryptedResultCard::save);
}

void EncryptedResultCard::showResult(const QString &summary, const QByteArray &encrypted,
                                     const QString &outputName, bool showBase64)
{
    m_encrypted = encrypted;
    m_outputName = outputName;

    m_summary->setText(summary + tr("<br>Arquivo de saída: <code>%1</code>").arg(outputName));
    m_output->setVisible(showBase64);
    m_copy->setVisible(showBase64);
    m_output->setText(showBase64 ? QString::fromLatin1(encrypted.toBase64()) : QString());
    show();
}

void EncryptedResultCard::clear()
{
    m_encrypted.clear();
    m_outputName.clear();
    m_output->clear();
    hide();
}

void EncryptedResultCard::save()
{
    if (m_encrypted.isEmpty())
        return;

    const QString folder = KeyTools::saveToChosenFolder(
        this, tr("Escolha a pasta para salvar o arquivo criptografado"),
        {{m_outputName, m_encrypted}});

    if (!folder.isEmpty())
        Popup::success(this, tr("Arquivo salvo em %1:\n\n%2").arg(folder, m_outputName));
}
