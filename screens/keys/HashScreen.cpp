#include "HashScreen.h"

#include <QButtonGroup>
#include <QClipboard>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include <optional>

#include "components/fields/FilePickerField.h"
#include "components/fields/SelectField.h"
#include "components/fields/TextAreaField.h"
#include "components/forms/SectionCard.h"
#include "components/labels/ProgressStatus.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "security/Hash.h"

HashScreen::HashScreen(QWidget *parent)
    : ContentPage(tr("Criar hash"),
                  tr("Calcula o resumo (hash) de um texto ou arquivo. O mesmo conteúdo sempre "
                     "gera o mesmo hash."),
                  parent)
    , m_algorithm(new SelectField(tr("Algoritmo")))
    , m_weakWarning(new TextLabel(
          tr("⚠ Algoritmo com colisões conhecidas: use apenas para verificação de integridade, "
             "nunca para segurança."),
          TextLabel::Variant::Caption))
    , m_textMode(new QRadioButton(tr("Texto")))
    , m_fileMode(new QRadioButton(tr("Arquivo")))
    , m_text(new TextAreaField(tr("Texto")))
    , m_file(new FilePickerField(tr("Arquivo")))
    , m_generate(new QPushButton(tr("Gerar hash")))
    , m_progress(new ProgressStatus())
    , m_result(new SectionCard(tr("Resultado")))
    , m_summary(new TextLabel(QString(), TextLabel::Variant::Body))
    , m_hex(new QLineEdit())
    , m_base64(new QLineEdit())
{
    for (const Hash::AlgorithmInfo &info : Hash::algorithms()) {
        const QString label = info.weak ? tr("%1  (inseguro)").arg(info.name) : info.name;
        m_algorithm->addOption(label, int(info.algorithm));
    }

    auto *modes = new QButtonGroup(this);
    modes->addButton(m_textMode);
    modes->addButton(m_fileMode);
    m_textMode->setChecked(true);

    auto *modeRow = new QHBoxLayout();
    modeRow->setSpacing(16);
    modeRow->addWidget(new TextLabel(tr("Gerar hash de:"), TextLabel::Variant::Caption));
    modeRow->addWidget(m_textMode);
    modeRow->addWidget(m_fileMode);
    modeRow->addStretch();

    m_text->setPlaceholder(tr("Digite ou cole o texto aqui (codificado em UTF-8)"));

    m_generate->setProperty("primary", true);
    m_generate->setCursor(Qt::PointingHandCursor);

    auto *settings = new SectionCard(tr("Configuração"));
    settings->addWidget(m_algorithm);
    settings->addWidget(m_weakWarning);
    settings->addLayout(modeRow);
    settings->addWidget(m_text);
    settings->addWidget(m_file);

    auto *actions = new QHBoxLayout();
    actions->addWidget(m_progress, 1);
    actions->addStretch();
    actions->addWidget(m_generate);
    settings->addLayout(actions);

    m_result->addWidget(m_summary);
    m_result->addWidget(new TextLabel(tr("Hexadecimal"), TextLabel::Variant::Caption));
    m_result->addWidget(createOutputRow(m_hex));
    m_result->addWidget(new TextLabel(tr("Base64"), TextLabel::Variant::Caption));
    m_result->addWidget(createOutputRow(m_base64));
    m_result->hide();

    addContent(settings);
    addContent(m_result);

    connect(m_algorithm, &SelectField::currentValueChanged, this, [this](const QVariant &value) {
        const auto algorithm = Hash::Algorithm(value.toInt());
        bool weak = false;
        for (const Hash::AlgorithmInfo &info : Hash::algorithms()) {
            if (info.algorithm == algorithm)
                weak = info.weak;
        }
        m_weakWarning->setVisible(weak);
        m_result->hide();
    });
    connect(m_fileMode, &QRadioButton::toggled, this, &HashScreen::updateMode);
    connect(m_generate, &QPushButton::clicked, this, &HashScreen::generate);

    m_weakWarning->hide();
    updateMode();
}

QWidget *HashScreen::createOutputRow(QLineEdit *output)
{
    output->setReadOnly(true);
    output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    output->setMinimumHeight(38);

    auto *copy = new QPushButton(tr("Copiar"));
    copy->setCursor(Qt::PointingHandCursor);
    copy->setMinimumHeight(38);
    connect(copy, &QPushButton::clicked, this, [this, output]() {
        QGuiApplication::clipboard()->setText(output->text());
        Popup::info(this, tr("Copiado para a área de transferência."));
    });

    auto *row = new QWidget();
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(output, 1);
    layout->addWidget(copy);
    return row;
}

bool HashScreen::isFileMode() const
{
    return m_fileMode->isChecked();
}

void HashScreen::updateMode()
{
    m_text->setVisible(!isFileMode());
    m_file->setVisible(isFileMode());
    m_result->hide();
}

void HashScreen::generate()
{
    const auto algorithm = Hash::Algorithm(m_algorithm->currentValue().toInt());

    if (!isFileMode()) {
        const QString text = m_text->text();
        if (text.isEmpty()) {
            Popup::warning(this, tr("Informe o texto."));
            return;
        }
        showResult(Hash::hash(algorithm, text.toUtf8()),
                   tr("texto (%1 caracteres)").arg(text.size()));
        return;
    }

    const QString path = m_file->filePath();
    if (path.isEmpty()) {
        Popup::warning(this, tr("Escolha o arquivo."));
        return;
    }

    setBusy(true);
    m_progress->start(tr("Calculando hash do arquivo…"));

    auto *watcher = new QFutureWatcher<std::optional<QByteArray>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, path]() {
        watcher->deleteLater();
        setBusy(false);

        const std::optional<QByteArray> digest = watcher->result();
        if (!digest) {
            Popup::error(this, tr("Não foi possível ler o arquivo:\n%1").arg(path));
            return;
        }
        showResult(*digest, QFileInfo(path).fileName());
    });

    watcher->setFuture(
        QtConcurrent::run([algorithm, path]() { return Hash::hashFile(algorithm, path); }));
}

void HashScreen::showResult(const QByteArray &digest, const QString &source)
{
    const auto algorithm = Hash::Algorithm(m_algorithm->currentValue().toInt());
    m_summary->setText(tr("<b>%1</b> de %2 — %3 bits")
                           .arg(Hash::name(algorithm), source.toHtmlEscaped())
                           .arg(digest.size() * 8));
    m_hex->setText(QString::fromLatin1(digest.toHex()));
    m_hex->setCursorPosition(0);
    m_base64->setText(QString::fromLatin1(digest.toBase64()));
    m_base64->setCursorPosition(0);
    m_result->show();
}

void HashScreen::setBusy(bool busy)
{
    m_generate->setEnabled(!busy);
    m_algorithm->setEnabled(!busy);
    m_textMode->setEnabled(!busy);
    m_fileMode->setEnabled(!busy);
    m_file->setEnabled(!busy);
    if (!busy)
        m_progress->stop();
}
