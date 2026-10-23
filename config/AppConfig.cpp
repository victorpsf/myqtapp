#include "AppConfig.h"

#include "Env.h"

namespace AppConfig {

QString authApiUrl()
{
    return Env::get("AUTH_API_URL");
}

QString personalApiUrl()
{
    return Env::get("PERSONAL_API_URL");
}

int httpTimeoutMs()
{
    bool ok = false;
    const int value = Env::get("HTTP_TIMEOUT_MS", "15000").toInt(&ok);
    return ok && value > 0 ? value : 15000;
}

} // namespace AppConfig
