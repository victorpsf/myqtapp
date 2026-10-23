#pragma once

#include <QByteArray>

namespace Random {

// Bytes aleatórios criptograficamente seguros (vazio em caso de falha)
QByteArray bytes(int size);

} // namespace Random
