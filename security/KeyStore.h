#pragma once

#include <QByteArray>
#include <QString>

// Arquivos de segurança em disco, no diretório da aplicação:
//   ~/.local/share/myqtapp/keys/client_public_key.pem          chave pública do cliente (enviada)
//   ~/.local/share/myqtapp/keys/server_<servico>_public_key.pem chave pública do servidor (recebida)
//   ~/.local/share/myqtapp/keys/local_<hash>.salt              salt para derivar a chave local
//   ~/.local/share/myqtapp/data/                               arquivos criptografados (AES-256-CBC)
//
// A chave privada NUNCA é gravada: fica apenas em memória (SecuritySession).
namespace KeyStore {

QString rootDirectory();
QString keysDirectory();
QString dataDirectory();

QString clientPublicKeyPath();
QString serverPublicKeyPath(const QString &service);

bool saveClientPublicKey(const QByteArray &pem);
QByteArray loadClientPublicKey();

bool saveServerPublicKey(const QString &service, const QByteArray &pem);
QByteArray loadServerPublicKey(const QString &service);

// Salt por usuário (não é segredo, só precisa ser estável entre execuções)
QByteArray loadOrCreateSalt(const QString &login);

bool writeFile(const QString &path, const QByteArray &data);
QByteArray readFile(const QString &path);

} // namespace KeyStore
