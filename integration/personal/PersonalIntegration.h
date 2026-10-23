#pragma once

#include <QObject>

#include "integration/HttpClient.h"

// Comunicação com o microserviço "pessoal" (PERSONAL_API_URL):
// cadastro e alteração de clientes. Adicione os métodos aqui seguindo
// o mesmo padrão do AuthIntegration, ex.:
//   void createCustomer(const CustomerRequest &, Callback<Customer>);
class PersonalIntegration : public HttpClient
{
    Q_OBJECT

public:
    explicit PersonalIntegration(QObject *parent = nullptr);
};
