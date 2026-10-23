#include "SecureEnvelope.h"

#include "security/OpenSsl.h"
#include "security/Random.h"
#include "security/symmetric/Aes256Cbc.h"

namespace SecureEnvelope {

std::optional<SealedMessage> seal(const QByteArray &plain, const RsaKey &recipientPublic,
                                  const RsaKey &senderPrivate)
{
    if (recipientPublic.isNull() || !senderPrivate.hasPrivateKey())
        return std::nullopt;

    QByteArray contentKey = Random::bytes(Aes256Cbc::SealKeySize);
    if (contentKey.isEmpty())
        return std::nullopt;

    SealedMessage message;
    message.data = Aes256Cbc::seal(plain, contentKey);
    message.key = recipientPublic.encrypt(contentKey);
    OpenSsl::cleanse(contentKey);

    if (message.data.isEmpty() || message.key.isEmpty())
        return std::nullopt;

    message.signature = senderPrivate.sign(message.key + message.data);
    if (message.signature.isEmpty())
        return std::nullopt;

    return message;
}

std::optional<QByteArray> open(const SealedMessage &message, const RsaKey &recipientPrivate,
                               const RsaKey &senderPublic)
{
    if (!recipientPrivate.hasPrivateKey() || senderPublic.isNull())
        return std::nullopt;

    if (!senderPublic.verify(message.key + message.data, message.signature))
        return std::nullopt;

    std::optional<QByteArray> contentKey = recipientPrivate.decrypt(message.key);
    if (!contentKey || contentKey->size() != Aes256Cbc::SealKeySize)
        return std::nullopt;

    std::optional<QByteArray> plain = Aes256Cbc::open(message.data, *contentKey);
    OpenSsl::cleanse(*contentKey);
    return plain;
}

} // namespace SecureEnvelope
