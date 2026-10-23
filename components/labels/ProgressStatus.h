#pragma once

#include <QWidget>

class QProgressBar;
class TextLabel;

// Barra indeterminada + mensagem, visível só durante uma operação
class ProgressStatus : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressStatus(QWidget *parent = nullptr);

    void start(const QString &message);
    void stop();

private:
    QProgressBar *m_bar;
    TextLabel *m_message;
};
