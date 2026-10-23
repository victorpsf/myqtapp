#pragma once

#include <QHash>
#include <QString>

// Carrega variáveis de um arquivo .env.
// Ordem de busca: pasta do executável, diretório atual.
// Variáveis de ambiente do sistema têm prioridade sobre o arquivo.
class Env
{
public:
    static bool load(const QString &filePath = QString());
    static QString get(const QString &key, const QString &defaultValue = QString());

private:
    static bool parseFile(const QString &filePath);

    static QHash<QString, QString> s_values;
};
