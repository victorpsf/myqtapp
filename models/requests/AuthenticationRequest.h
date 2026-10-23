#pragma once

#include <QJsonObject>
#include <QString>

// Dados informados pelo usuário ao longo do fluxo de login:
// 1) usuário/e-mail  2) senha  3) código 2FA
class AuthenticationRequest
{
public:
    QString login() const { return m_login; }
    void setLogin(const QString &login) { m_login = login.trimmed(); }

    QString password() const { return m_password; }
    void setPassword(const QString &password) { m_password = password; }

    QString twoFactorCode() const { return m_twoFactorCode; }
    void setTwoFactorCode(const QString &code) { m_twoFactorCode = code.trimmed(); }

    // Token devolvido pelo auth após a senha, usado para validar/reenviar o 2FA
    QString twoFactorToken() const { return m_twoFactorToken; }
    void setTwoFactorToken(const QString &token) { m_twoFactorToken = token; }

    bool isEmail() const { return m_login.contains('@'); }

    QJsonObject toIdentifyJson() const;
    QJsonObject toLoginJson() const;
    QJsonObject toVerifyTwoFactorJson() const;
    QJsonObject toResendTwoFactorJson() const;

    void clearPassword();
    void clear();

private:
    QString m_login;
    QString m_password;
    QString m_twoFactorCode;
    QString m_twoFactorToken;
};
