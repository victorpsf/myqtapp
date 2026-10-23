#include "Random.h"

#include <openssl/rand.h>

#include "OpenSsl.h"

namespace Random {

QByteArray bytes(int size)
{
    QByteArray buffer(size, Qt::Uninitialized);
    if (RAND_bytes(OpenSsl::bytes(buffer), size) != 1) {
        qWarning("Random: %s", qPrintable(OpenSsl::lastError()));
        return QByteArray();
    }
    return buffer;
}

} // namespace Random
