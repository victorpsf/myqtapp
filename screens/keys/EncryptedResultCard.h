#pragma once

#include "components/forms/SectionCard.h"

class QPushButton;
class TextAreaField;
class TextLabel;

// Resultado de uma criptografia: resumo, base64 (texto) e "Salvar em pasta…"
class EncryptedResultCard : public SectionCard
{
    Q_OBJECT

public:
    explicit EncryptedResultCard(QWidget *parent = nullptr);

    // `summary` aceita HTML simples; `showBase64` exibe o conteúdo para copiar
    void showResult(const QString &summary, const QByteArray &encrypted, const QString &outputName,
                    bool showBase64);
    void clear();

private:
    void save();

    TextLabel *m_summary;
    TextAreaField *m_output;
    QPushButton *m_copy;
    QPushButton *m_save;

    QByteArray m_encrypted;
    QString m_outputName;
};
