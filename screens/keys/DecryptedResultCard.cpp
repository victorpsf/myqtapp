#include "DecryptedResultCard.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QStringDecoder>

#include "components/fields/TextAreaField.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "screens/keys/KeyToolsCommon.h"
#include "security/OpenSsl.h"

namespace {

// Texto só é exibido se for UTF-8 válido; caso contrário é tratado como binário
std::optional<QString> decodeUtf8(const QByteArray &data)
{
    QStringDecoder decoder(QStringDecoder::Utf8);
    QString text = decoder(data);
    if (decoder.hasError())
        return std::nullopt;
    return text;
}

} // namespace

DecryptedResultCard::DecryptedResultCard(QWidget *parent)
    : SectionCard(tr("Resultado"), parent)
    , m_summary(new TextLabel(QString(), TextLabel::Variant::Body))
    , m_output(new TextAreaField(tr("Texto descriptografado")))
    , m_copy(new QPushButton(tr("Copiar")))
    , m_save(new QPushButton(tr("Salvar em pasta…")))
{
    m_output->setReadOnly(true);

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
    connect(m_save, &QPushButton::clicked, this, &DecryptedResultCard::save);
}

DecryptedResultCard::~DecryptedResultCard()
{
    OpenSsl::cleanse(m_data);
}

void DecryptedResultCard::showContent(const QString &summary,
                                      const ContentPackage::Content &content,
                                      const QString &fallbackName)
{
    clear();
    m_data = content.data;

    const std::optional<QString> text =
        content.isFile() ? std::nullopt : decodeUtf8(content.data);

    if (content.isFile())
        m_outputName = content.fileName;
    else if (text)
        m_outputName = QStringLiteral("texto_%1.txt").arg(KeyTools::timestamp());
    else
        m_outputName = fallbackName;

    QString details = summary;
    details += tr("<br>Tamanho: %1").arg(QLocale().formattedDataSize(m_data.size()));
    if (content.isFile())
        details += tr("<br>Arquivo original: <code>%1</code>").arg(content.fileName.toHtmlEscaped());
    else if (!text)
        details += tr("<br>O conteúdo não é texto; salve-o como arquivo.");
    m_summary->setText(details);

    m_output->setVisible(text.has_value());
    m_copy->setVisible(text.has_value());
    if (text)
        m_output->setText(*text);

    show();
}

void DecryptedResultCard::clear()
{
    OpenSsl::cleanse(m_data);
    m_outputName.clear();
    m_output->clear();
    hide();
}

void DecryptedResultCard::save()
{
    if (m_outputName.isEmpty())
        return;

    const QString folder = KeyTools::saveToChosenFolder(
        this, tr("Escolha a pasta para salvar o conteúdo descriptografado"),
        {{m_outputName, m_data}});

    if (!folder.isEmpty())
        Popup::success(this, tr("Arquivo salvo em %1:\n\n%2").arg(folder, m_outputName));
}
