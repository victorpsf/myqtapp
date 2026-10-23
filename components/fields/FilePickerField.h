#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;

// Seleção de um arquivo do computador
class FilePickerField : public QWidget
{
    Q_OBJECT

public:
    explicit FilePickerField(const QString &label, QWidget *parent = nullptr);

    QString filePath() const { return m_path; }
    void clear();

signals:
    void fileSelected(const QString &path);

private:
    void choose();

    QLineEdit *m_display;
    QPushButton *m_button;
    QString m_path;
};
