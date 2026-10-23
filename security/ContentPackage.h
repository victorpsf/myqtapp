#pragma once

#include <QByteArray>
#include <QString>

// Conteúdo que vai dentro da criptografia: o nome original do arquivo segue
// cifrado junto com os dados, assim o arquivo de saída pode ter o nome em hash
// e a descriptografia ainda restaura o nome original.
//
// Formato: "MQAPKG" | versão (1) | tamanho do nome (2, big-endian) | nome UTF-8 | dados
// Texto é empacotado com o nome vazio.
namespace ContentPackage {

struct Content
{
    QString fileName; // vazio = texto
    QByteArray data;

    bool isFile() const { return !fileName.isEmpty(); }
};

QByteArray pack(const Content &content);

// Dados sem o cabeçalho (criptografados antes deste formato) voltam como estão, sem nome
Content unpack(const QByteArray &packed);

} // namespace ContentPackage
