#pragma once

#include <QString>

// Acesso tipado às configurações vindas do .env
namespace AppConfig {

QString authApiUrl();
QString personalApiUrl();
int httpTimeoutMs();

} // namespace AppConfig
