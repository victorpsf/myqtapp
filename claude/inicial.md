configure uma aplicação qt com c++, organize as pastas com:
  - models
  - models/entity
  - models/requests
  - models/responses
  - integration
  - integration/auth
  - integration/personal
  - components
  - components/forms
  - components/fields
  - components/labels
  - components/popups
  - .env
  - config
  - screens

Iniciei o programa com uma tela de login, que pede o usuário/e-mail, que tem um botão de continuar, armazene em uma classe AuthenticationRequest em models/requests.

na proxima tela apresente o usuário ou e-mail informado e peça a senha, que tem um botão de logar ou voltar (botão de voltar volta para a tela de informar usuário/e-mail) armazene em uma classe AuthenticationRequest em models/requests.

após clicar em logar mande para uma tela que pede código de 2fa, e botão de verificar e reenviar codigo 2fa, armazene em uma classe AuthenticationRequest em models/requests.

modo de funcionamento crie classes para comunicação http na pasta de integration.
crie um .env e obtenha o dominio por lá, obs: utilizo microserviços então para cada microservico vai se criar uma pasta dentro de integration, cada microserviço tem um dominio proprio.

microserviços:
  - auth (responsavel pela autenticação)
  - pessoal (responsavel por realizar cadastros e alteração dos clientes)

Com base nos inputs das telas crie os metodos de comunicação com o microserviço auth, lembrando que cada tela tem um fluxo no sistema. 

os metodos podem retornar erro em um json com estrutura de crie uma classe chamada IntegrationResponse o data pode ser uma classe informada, utilize generic:

``` json
{
    "error": "",
    "data": null
}
```

o erro mostra o que aconteceu, apresente em um popup a mensagem retornada.

para isso crie component de pop up (deixe configuravel) onde vai ser de warning/error/info/success.

no caso do erro retornado pela api a configuração vai ser de error.