#pragma once

#include <QByteArray>

#include <optional>

#include "Rsa.h"

// Mensagem crítica trafegada entre cliente e servidor (criptografia híbrida).
// RSA-8192 sozinho só cifra ~958 bytes, então:
//   key       = RSA-OAEP(chave AES aleatória, chave pública do destinatário)
//   data      = AES-256-CBC + HMAC (Aes256Cbc::seal) do conteúdo
//   signature = RSA-PSS(key | data, chave privada do remetente)
struct SealedMessage
{
    QByteArray key;
    QByteArray data;
    QByteArray signature;
};

namespace SecureEnvelope {

std::optional<SealedMessage> seal(const QByteArray &plain, const RsaKey &recipientPublic,
                                  const RsaKey &senderPrivate);

// Verifica a assinatura do remetente antes de descriptografar
std::optional<QByteArray> open(const SealedMessage &message, const RsaKey &recipientPrivate,
                               const RsaKey &senderPublic);

} // namespace SecureEnvelope
