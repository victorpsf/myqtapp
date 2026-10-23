#pragma once

#include "components/forms/SectionCard.h"
#include "security/ContentPackage.h"

class QPushButton;
class TextAreaField;
class TextLabel;

// Resultado de uma descriptografia: texto legível (com "Copiar") ou arquivo para salvar.
// O conteúdo é apagado da memória ao limpar ou destruir o card.
class DecryptedResultCard : public SectionCard
{
    Q_OBJECT

public:
    explicit DecryptedResultCard(QWidget *parent = nullptr);
    ~DecryptedResultCard() override;

    // `fallbackName` é usado quando o conteúdo não traz o nome original
    void showContent(const QString &summary, const ContentPackage::Content &content,
                     const QString &fallbackName);
    void clear();

private:
    void save();

    TextLabel *m_summary;
    TextAreaField *m_output;
    QPushButton *m_copy;
    QPushButton *m_save;

    QByteArray m_data;
    QString m_outputName;
};
