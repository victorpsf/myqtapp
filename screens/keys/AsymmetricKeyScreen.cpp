#include "AsymmetricKeyScreen.h"

#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QPushButton>
#include <QtConcurrent/QtConcurrentRun>

#include "components/fields/InputField.h"
#include "components/fields/SelectField.h"
#include "components/fields/TextAreaField.h"
#include "components/forms/SectionCard.h"
#include "components/labels/ProgressStatus.h"
#include "components/labels/TextLabel.h"
#include "components/popups/Popup.h"
#include "screens/keys/KeyToolsCommon.h"
#include "security/Hash.h"
#include "security/OpenSsl.h"

using namespace AsymmetricKeyGenerator;

AsymmetricKeyScreen::AsymmetricKeyScreen(QWidget *parent)
    : ContentPage(tr("Criar chaves assimétricas"),
                  tr("Gera um par de chaves (pública e privada). A chave privada pode ser "
                     "protegida com senha."),
                  parent)
    , m_algorithm(new SelectField(tr("Algoritmo")))
    , m_algorithmHint(new TextLabel(QString(), TextLabel::Variant::Caption))
    , m_rsaSize(new SelectField(tr("Tamanho da chave")))
    , m_curve(new SelectField(tr("Curva")))
    , m_password(new InputField(tr("Senha da chave privada (opcional)")))
    , m_confirm(new InputField(tr("Confirmar senha")))
    , m_generate(new QPushButton(tr("Gerar chaves")))
    , m_cancel(new QPushButton(tr("Cancelar")))
    , m_progress(new ProgressStatus())
    , m_result(new SectionCard(tr("Chaves geradas")))
    , m_summary(new TextLabel(QString(), TextLabel::Variant::Body))
    , m_publicKey(new TextAreaField(tr("Chave pública (PEM)")))
    , m_save(new QPushButton(tr("Salvar chaves em pasta…")))
{
    for (const AlgorithmInfo &info : algorithms())
        m_algorithm->addOption(info.name, int(info.algorithm), info.description);

    for (int bits : rsaSizes()) {
        QString label = tr("%1 bits").arg(bits);
        if (bits < 2048)
            label += tr("  (inseguro, só para testes)");
        else if (bits == 4096)
            label += tr("  (recomendado)");
        else if (bits == 8192)
            label += tr("  (muito lento para gerar)");
        m_rsaSize->addOption(label, bits);
    }
    m_rsaSize->setCurrentValue(4096);

    for (const QString &curve : curves())
        m_curve->addOption(curve, curve);

    m_password->setPasswordMode(true);
    m_confirm->setPasswordMode(true);

    m_generate->setProperty("primary", true);
    m_generate->setCursor(Qt::PointingHandCursor);
    m_cancel->setCursor(Qt::PointingHandCursor);
    m_cancel->hide();

    auto *settings = new SectionCard(tr("Configuração"));
    settings->addWidget(m_algorithm);
    settings->addWidget(m_algorithmHint);
    settings->addWidget(m_rsaSize);
    settings->addWidget(m_curve);
    settings->addWidget(m_password);
    settings->addWidget(m_confirm);
    settings->addWidget(new TextLabel(
        tr("Com senha, a chave privada é salva criptografada (PKCS#8 + AES-256-CBC). "
           "Sem senha, qualquer pessoa com acesso ao arquivo poderá usá-la."),
        TextLabel::Variant::Caption));

    auto *actions = new QHBoxLayout();
    actions->addWidget(m_progress, 1);
    actions->addStretch();
    actions->addWidget(m_cancel);
    actions->addWidget(m_generate);
    settings->addLayout(actions);

    m_publicKey->setReadOnly(true);
    m_publicKey->setMonospace(true);
    m_publicKey->setVisibleLines(8);
    m_save->setProperty("primary", true);
    m_save->setCursor(Qt::PointingHandCursor);

    auto *saveRow = new QHBoxLayout();
    saveRow->addStretch();
    saveRow->addWidget(m_save);

    m_result->addWidget(m_summary);
    m_result->addWidget(m_publicKey);
    m_result->addLayout(saveRow);
    m_result->hide();

    addContent(settings);
    addContent(m_result);

    connect(m_algorithm, &SelectField::currentValueChanged, this,
            &AsymmetricKeyScreen::updateAlgorithmOptions);
    connect(m_generate, &QPushButton::clicked, this, &AsymmetricKeyScreen::generate);
    connect(m_cancel, &QPushButton::clicked, this, &AsymmetricKeyScreen::cancel);
    connect(m_save, &QPushButton::clicked, this, &AsymmetricKeyScreen::save);

    updateAlgorithmOptions();
}

AsymmetricKeyScreen::~AsymmetricKeyScreen()
{
    if (m_cancelFlag)
        m_cancelFlag->store(true);
    OpenSsl::cleanse(m_keys.privatePem);
}

Options AsymmetricKeyScreen::options() const
{
    Options result;
    result.algorithm = Algorithm(m_algorithm->currentValue().toInt());
    result.rsaBits = m_rsaSize->currentValue().toInt();
    result.curve = m_curve->currentValue().toString();
    return result;
}

void AsymmetricKeyScreen::updateAlgorithmOptions()
{
    const Algorithm algorithm = Algorithm(m_algorithm->currentValue().toInt());
    m_rsaSize->setVisible(algorithm == Algorithm::Rsa);
    m_curve->setVisible(algorithm == Algorithm::Ec);

    for (const AlgorithmInfo &info : algorithms()) {
        if (info.algorithm == algorithm)
            m_algorithmHint->setText(info.description);
    }
}

void AsymmetricKeyScreen::generate()
{
    const QString password = m_password->text();
    if (password != m_confirm->text()) {
        Popup::warning(this, tr("As senhas não conferem."));
        m_confirm->focusInput();
        return;
    }

    if (password.isEmpty()
        && !Popup::display(this, {PopupType::Warning, tr("Gerar sem senha?"),
                                  tr("A chave privada será salva sem proteção. Deseja continuar?"),
                                  tr("Continuar"), tr("Cancelar")})) {
        return;
    }

    const Options opts = options();
    clearResult();
    setBusy(true);
    m_progress->start(tr("Gerando %1…").arg(displayName(opts)));

    auto flag = std::make_shared<std::atomic_bool>(false);
    m_cancelFlag = flag;

    auto *watcher = new QFutureWatcher<KeyPair>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, flag, opts]() {
        watcher->deleteLater();
        if (flag != m_cancelFlag)
            return;
        m_cancelFlag.reset();
        setBusy(false);

        KeyPair keys = watcher->result();
        if (flag->load()) {
            Popup::info(this, tr("Geração cancelada."));
            return;
        }
        if (!keys.isValid()) {
            Popup::error(this, keys.error);
            return;
        }

        m_keys = keys;
        m_filePrefix = QStringLiteral("%1_%2").arg(shortName(opts), KeyTools::timestamp());

        const QString fingerprint = Hash::sha256Hex(m_keys.publicPem);
        m_summary->setText(
            tr("<b>%1</b> — chave privada %2<br>SHA-256 da chave pública: <code>%3</code>")
                .arg(displayName(opts),
                     m_keys.privatePem.contains("ENCRYPTED") ? tr("protegida por senha")
                                                             : tr("sem senha"),
                     fingerprint.left(32) + QStringLiteral("…")));
        m_publicKey->setText(QString::fromLatin1(m_keys.publicPem));
        m_result->show();
    });

    QByteArray passphrase = password.toUtf8();
    watcher->setFuture(QtConcurrent::run([opts, passphrase, flag]() mutable {
        KeyPair keys = AsymmetricKeyGenerator::generate(opts, passphrase, flag.get());
        OpenSsl::cleanse(passphrase);
        return keys;
    }));
    OpenSsl::cleanse(passphrase);
}

void AsymmetricKeyScreen::cancel()
{
    if (m_cancelFlag)
        m_cancelFlag->store(true);
    m_progress->start(tr("Cancelando…"));
}

void AsymmetricKeyScreen::save()
{
    if (!m_keys.isValid())
        return;

    const QString folder = KeyTools::saveToChosenFolder(
        this, tr("Escolha a pasta para salvar as chaves"),
        {
            {m_filePrefix + QStringLiteral("_public_key.pem"), m_keys.publicPem},
            {m_filePrefix + QStringLiteral("_private_key.pem"), m_keys.privatePem},
        });

    if (!folder.isEmpty()) {
        Popup::success(this, tr("Chaves salvas em %1:\n\n%2_public_key.pem\n%2_private_key.pem")
                                 .arg(folder, m_filePrefix));
    }
}

void AsymmetricKeyScreen::setBusy(bool busy)
{
    m_generate->setEnabled(!busy);
    m_cancel->setVisible(busy);
    m_algorithm->setEnabled(!busy);
    m_rsaSize->setEnabled(!busy);
    m_curve->setEnabled(!busy);
    m_password->setEnabled(!busy);
    m_confirm->setEnabled(!busy);
    if (!busy)
        m_progress->stop();
}

void AsymmetricKeyScreen::clearResult()
{
    OpenSsl::cleanse(m_keys.privatePem);
    m_keys = KeyPair();
    m_publicKey->clear();
    m_result->hide();
}
