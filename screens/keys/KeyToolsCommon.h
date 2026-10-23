#pragma once

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>

#include <optional>

class QWidget;

namespace KeyTools {

using OutputFile = QPair<QString, QByteArray>; // nome do arquivo, conteúdo

// Abre o seletor de pasta e grava os arquivos (pergunta antes de sobrescrever).
// Retorna a pasta usada, ou vazio se o usuário cancelou ou houve erro.
QString saveToChosenFolder(QWidget *parent, const QString &dialogTitle,
                           const QList<OutputFile> &files);

QString timestamp(); // yyyyMMdd_HHmmss

// Hash SHA-256 do nome original, preservando a extensão (relatorio.pdf -> <hash>.pdf)
QString encryptedFileName(const QString &originalPath);

// Lê o arquivo inteiro. Pode ser chamada fora da thread da interface.
std::optional<QByteArray> readWholeFile(const QString &path, QString *error);

// Lê um arquivo de chave PEM (recusa arquivos grandes demais para serem uma chave)
std::optional<QByteArray> readKeyFile(const QString &path, QString *error);

// Base64 colado pelo usuário (ignora espaços e quebras de linha)
std::optional<QByteArray> decodeBase64(const QString &text);

} // namespace KeyTools
