#pragma once

#include <QByteArray>
#include <QString>

// Criptografia com chave pública (PEM) e descriptografia com a chave privada.
// O conteúdo é cifrado com AES-256-GCM usando uma chave aleatória de uso único;
// só essa chave depende do par assimétrico (assim não há limite de tamanho):
//   RSA          -> chave AES cifrada com RSA-OAEP-SHA256
//   EC / X25519  -> chave AES derivada por ECDH com uma chave efêmera + HKDF-SHA256
// Ed25519/Ed448 são só de assinatura e não servem para criptografar.
//
// Formato gerado:
//   "MQAPKE" | versão (1) | esquema (1) | tamanho do bloco (2, big-endian) | bloco
//   | nonce (12) | ciphertext | tag (16)
// bloco = chave AES cifrada (RSA) ou chave pública efêmera em DER (ECDH).
// Tudo antes do ciphertext é autenticado pelo GCM.
namespace PublicKeyEncryption {

struct Result
{
    QByteArray data;
    QString error;

    bool isSuccess() const { return error.isEmpty(); }
};

struct KeyInfo
{
    QString description; // ex.: "RSA 4096 bits", "EC P-256"
    bool isPrivate = false;
    bool isEncrypted = false; // chave privada protegida por senha
    bool canEncrypt = false;
    QString error;
};

// Identifica o tipo da chave em um arquivo PEM (sem pedir senha)
KeyInfo inspect(const QByteArray &pem);

// Bloqueantes: chame fora da thread da interface
Result encrypt(const QByteArray &publicPem, const QByteArray &plain);
Result decrypt(const QByteArray &privatePem, const QByteArray &passphrase,
               const QByteArray &encrypted);

bool looksEncrypted(const QByteArray &data);

} // namespace PublicKeyEncryption
