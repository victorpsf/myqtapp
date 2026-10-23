#include "AuthenticationRequest.h"

QJsonObject AuthenticationRequest::toIdentifyJson() const
{
    return {
        {"login", m_login},
    };
}

QJsonObject AuthenticationRequest::toLoginJson() const
{
    return {
        {"login", m_login},
        {"password", m_password},
    };
}

QJsonObject AuthenticationRequest::toVerifyTwoFactorJson() const
{
    return {
        {"login", m_login},
        {"token", m_twoFactorToken},
        {"code", m_twoFactorCode},
    };
}

QJsonObject AuthenticationRequest::toResendTwoFactorJson() const
{
    return {
        {"login", m_login},
        {"token", m_twoFactorToken},
    };
}

void AuthenticationRequest::clearPassword()
{
    m_password.clear();
    m_twoFactorCode.clear();
    m_twoFactorToken.clear();
}

void AuthenticationRequest::clear()
{
    m_login.clear();
    clearPassword();
}
