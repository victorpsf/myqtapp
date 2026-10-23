#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include <optional>

#include "Aes256Cbc.h"

// Criptografia simétrica a partir de uma senha (texto ou arquivo).
// A chave é derivada com PBKDF2-HMAC-SHA256 e um salt aleatório.
//
// Formato gerado (tudo que é preciso para descriptografar, exceto a senha):
//   "MQAENC" | versão (1) | algoritmo (1) | iterações (4, big-endian) | salt (16) | IV/nonce
//   | ciphertext | tag
// tag = HMAC-SHA256 (32) nos modos CBC, ou tag AEAD (16) em GCM/ChaCha20-Poly1305.
// O cabeçalho entra na autenticação, então qualquer alteração é detectada.
namespace PasswordEncryption {

enum class Algorithm : quint8 {
    Aes256Cbc = 1,
    Aes192Cbc = 2,
    Aes128Cbc = 3,
    Aes256Gcm = 4,
    ChaCha20Poly1305 = 5,
};

struct AlgorithmInfo
{
    Algorithm algorithm;
    QString name;
    QString description;
};

QList<AlgorithmInfo> algorithms();
QString name(Algorithm algorithm);

QByteArray encrypt(Algorithm algorithm, const QByteArray &plain, const QByteArray &password,
                   int iterations = Aes256Cbc::DefaultIterations);

std::optional<QByteArray> decrypt(const QByteArray &encrypted, const QByteArray &password);

} // namespace PasswordEncryption
