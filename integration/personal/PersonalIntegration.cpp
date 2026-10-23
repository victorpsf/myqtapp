#include "PersonalIntegration.h"

#include "config/AppConfig.h"

PersonalIntegration::PersonalIntegration(QObject *parent)
    : HttpClient(AppConfig::personalApiUrl(), parent)
{
}
