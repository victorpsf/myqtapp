#pragma once

#include <QByteArray>

#include <optional>

// AES-256-CBC (padding PKCS#7).
//
// encrypt/decrypt: operação "crua", você controla chave e IV.
// seal/open: formato pronto para salvar em disco ou trafegar:
//     IV (16) | ciphertext | HMAC-SHA256(IV | ciphertext) (32)
//   usando uma chave de 64 bytes: 32 para o AES + 32 para o HMAC.
//   O HMAC garante que o conteúdo não foi alterado (CBC sozinho não garante).
namespace Aes256Cbc {

constexpr int KeySize = 32;
constexpr int IvSize = 16;
constexpr int MacSize = 32;
constexpr int SealKeySize = KeySize + MacSize;
constexpr int SaltSize = 16;
constexpr int DefaultIterations = 310000;

QByteArray encrypt(const QByteArray &plain, const QByteArray &key, const QByteArray &iv);
std::optional<QByteArray> decrypt(const QByteArray &cipher, const QByteArray &key,
                                  const QByteArray &iv);

QByteArray seal(const QByteArray &plain, const QByteArray &sealKey);
std::optional<QByteArray> open(const QByteArray &sealed, const QByteArray &sealKey);

// PBKDF2-HMAC-SHA256: transforma a senha do usuário em chave
QByteArray deriveKey(const QByteArray &password, const QByteArray &salt,
                     int iterations = DefaultIterations, int length = SealKeySize);

} // namespace Aes256Cbc
