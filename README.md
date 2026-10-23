# myqtapp

Aplicação Qt 6 (Widgets) em C++17 com fluxo de login em 3 etapas: usuário/e-mail → senha → 2FA.

> Passo a passo de instalação e de ambiente de desenvolvimento para Linux, macOS e Windows: [docs/INSTALACAO.md](docs/INSTALACAO.md).

## Build

```bash
make run                              # configura (Debug), compila e executa
make debug                            # compila e abre no gdb
make run QT_PATH=~/Qt/6.x/gcc_64      # se o Qt não for o do sistema
make clean                            # apaga a pasta build/
```

Sem o Makefile: `cmake -S . -B build && cmake --build build --target run`.
Não rode `cmake .` na raiz — isso sobrescreve o `Makefile` do projeto.

O `.env` é copiado para a pasta do build ao rodar o CMake. Variáveis de ambiente do sistema têm prioridade sobre o arquivo.

## Estrutura

| Pasta | Conteúdo |
|---|---|
| `config/` | `Env` (lê o `.env`), `AppConfig` (domínios, timeout), `AppStyle` (stylesheet) |
| `models/entity/` | Entidades (`User`) |
| `models/requests/` | `AuthenticationRequest` — dados das 3 telas de login |
| `models/responses/` | `IntegrationResponse<T>` (envelope `{error, data}`) e os `data` de cada endpoint |
| `integration/` | `HttpClient` base; uma pasta por microserviço, cada uma com seu domínio |
| `integration/auth/` | `AuthIntegration` — `AUTH_API_URL` |
| `integration/personal/` | `PersonalIntegration` — `PERSONAL_API_URL` |
| `components/` | `forms/FormCard`, `fields/InputField`, `labels/TextLabel`, `popups/Popup` |
| `components/menus/` | `TopBar` (altura fixa, perfil, sair) e `SideMenu` (recolhível) |
| `security/` | `Hash` (SHA-256/512), `Random`, `KeyStore` (chaves em disco), `SecuritySession` (chaves em memória) |
| `security/symmetric/` | `Aes256Cbc` — AES-256-CBC + HMAC-SHA256, PBKDF2 |
| `security/asymmetric/` | `RsaKey` (RSA-8192, OAEP/PSS), `SecureEnvelope` (criptografia híbrida), `AsymmetricKeyGenerator`, `PublicKeyEncryption` |
| `screens/` | Telas: identificação, senha, 2FA, home e janela de perfil |

## Pacotes (AppImage, .deb, .dmg, .exe)

```bash
make package            # todos os formatos do sistema atual (scripts/package.sh)
make appimage           # só o AppImage
make deb                # só o .deb
```

| Sistema | Comando | Gera em `dist/` |
|---|---|---|
| Linux | `scripts/package.sh` | `myqtapp-<versão>-x86_64.AppImage` e `myqtapp_<versão>_amd64.deb` |
| macOS | `QT_PATH=~/Qt/6.8.3/macos scripts/package.sh` | `myqtapp-<versão>-Darwin.dmg` |
| Windows (PowerShell) | `scripts\package.cmd` ou `scripts\package.ps1 -QtPath C:\Qt\6.8.3\msvc2022_64` | instalador `myqtapp-<versão>-win64.exe` e `.zip` portátil |
| Windows (Git Bash) | `QT_PATH=C:/Qt/6.8.3/msvc2022_64 scripts/package.sh` | o mesmo |

Cada formato só pode ser gerado no próprio sistema. Para gerar todos a partir do Linux, suba o projeto no
GitHub e rode o workflow **Pacotes** (`.github/workflows/packages.yml`) em *Actions → Run workflow* ou
criando uma tag `v*`; os arquivos ficam nos *Artifacts* da execução.

Requisitos:

- **Linux:** `sudo apt install cmake build-essential qt6-base-dev qt6-base-dev-tools libssl-dev dpkg-dev curl`
  (e `qt6-wayland` para Wayland nativo no AppImage). O AppImage baixa o `linuxdeploy` na primeira execução
  e embute Qt e OpenSSL; o `.deb` usa o Qt do sistema (declara as dependências automaticamente).
- **macOS:** Qt 6.5+ pelo instalador oficial e `brew install cmake openssl@3`.
- **Windows:** Qt 6.5+ (MSVC) pelo instalador oficial, Visual Studio, CMake, [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)
  e NSIS (`winget install NSIS.NSIS`). O `package.ps1` acha sozinho o Qt mais novo em `C:\Qt` e o OpenSSL em
  `C:\Program Files\OpenSSL-Win64`; veja as opções com `Get-Help scripts\package.ps1 -Detailed`.

O `.env` do projeto (ou o `.env.example`, se não houver) é embutido; use `ENV_FILE=caminho/.env` para outro.
Para trocar os domínios depois de instalado, crie `~/.config/myqtapp/.env` (ou, no AppImage, um `.env` ao
lado do arquivo `.AppImage`); variáveis de ambiente continuam tendo prioridade.
Defina `PACKAGE_MAINTAINER="Nome <email>"` para o campo de mantenedor do `.deb`.

## Segurança

Dependência: OpenSSL (`sudo apt install libssl-dev`).

1. **Login (após o 2FA):** `SecuritySession::begin()` gera em segundo plano o par **RSA-8192** do cliente
   e deriva da senha (PBKDF2-SHA256, 310 mil iterações) a chave local do **AES-256-CBC**. A senha é
   descartada em seguida. Chave privada e chave local ficam **só em memória** e são apagadas no logout.
2. **Handshake:** ao abrir o perfil, o cliente envia a chave pública e recebe a do servidor.
3. **Mensagens críticas:** o servidor responde com `{key, data, signature}`: uma chave AES aleatória cifrada
   com RSA-OAEP, o conteúdo em AES-256-CBC + HMAC e uma assinatura RSA-PSS do servidor. O cliente verifica a
   assinatura antes de descriptografar.

Arquivos em `~/.local/share/myqtapp/`:

| Arquivo | Conteúdo |
|---|---|
| `keys/client_public_key.pem` | chave pública do cliente (enviada) |
| `keys/server_auth_public_key.pem` | chave pública do servidor `auth` (recebida) |
| `keys/local_<hash>.salt` | salt da derivação da chave local, por usuário |
| `data/*` | arquivos salvos com `SecuritySession::saveEncryptedFile()` |

## Menu Segurança

| Tela | Opções | Saída |
|---|---|---|
| Criar chaves (assimétrica) | RSA (512, 1024, 2048, 4096, 8192), ECDSA (P-256, P-384, P-521, secp256k1), Ed25519, Ed448; senha opcional | `<algoritmo>_<data>_public_key.pem` e `..._private_key.pem` (PKCS#8, AES-256-CBC com senha) |
| Criptografar (assimétrica) | chave pública `.pem` (RSA ≥ 1024 bits ou ECDSA); texto ou arquivo | arquivo: `sha256(nome original).<extensão>`; texto: base64 / `texto_<data>.enc` |
| Descriptografar (assimétrica) | chave privada `.pem` + senha da chave (se houver); base64 ou arquivo | texto na tela ou arquivo com o **nome original** |
| Criptografar (simétrica) | AES-256-CBC, AES-256-GCM, ChaCha20-Poly1305, AES-192-CBC, AES-128-CBC; senha; texto ou arquivo | arquivo: `sha256(nome original).<extensão>`; texto: base64 / `texto_<data>.enc` |
| Descriptografar (simétrica) | senha (algoritmo detectado pelo cabeçalho); base64 ou arquivo | texto na tela ou arquivo com o **nome original** |
| Criar hash | SHA-256/384/224/512, SHA3-256/512, BLAKE2b-512, SHA-1 e MD5 (marcados inseguros); texto ou arquivo | hexadecimal e base64, com botão copiar |

Antes de criptografar, o conteúdo é empacotado com o nome original do arquivo (`security/ContentPackage`:
`MQAPKG | versão | tamanho do nome | nome | dados`). O nome vai cifrado junto, então o arquivo de saída
pode ter o nome em hash e a descriptografia restaura o nome original.

Formatos:

- **Simétrica** (`security/symmetric/PasswordEncryption`): `MQAENC | versão | algoritmo | iterações | salt | IV | ciphertext | tag`.
  A chave vem da senha via PBKDF2-SHA256. O cabeçalho é autenticado (HMAC nos modos CBC, tag AEAD em GCM/ChaCha20).
- **Assimétrica** (`security/asymmetric/PublicKeyEncryption`): `MQAPKE | versão | esquema | tamanho | bloco | nonce | ciphertext | tag`.
  O conteúdo é cifrado com AES-256-GCM. Com RSA, a chave AES aleatória vai cifrada com RSA-OAEP-SHA256 no bloco;
  com curva elíptica, o bloco é uma chave pública efêmera e a chave AES sai de ECDH + HKDF-SHA256.
  Ed25519/Ed448 são só de assinatura e não podem ser usadas para criptografar.

## Modo mock (atual)

As requisições do `AuthIntegration` estão comentadas e os métodos retornam respostas fixas (com ~400 ms de atraso) para testar as telas:

| Tela | Sucesso | Erro (popup) |
|---|---|---|
| Usuário/e-mail | qualquer valor | `erro` |
| Senha | qualquer valor | `errada` |
| 2FA | `123456` | qualquer outro código |
| Reenviar código | sempre | — |
| Perfil (handshake) | o mock gera uma chave de servidor de 2048 bits e devolve o perfil criptografado e assinado | — |

Para usar a API real, descomente as linhas `post<...>` em `integration/auth/AuthIntegration.cpp` e remova o bloco de mock.

## Contrato esperado do microserviço auth

Toda resposta segue `{ "error": "", "data": ... }`. Se `error` não estiver vazio, a mensagem é exibida em um popup do tipo **error**.

| Tela | Método | Endpoint | Body | `data` |
|---|---|---|---|---|
| Usuário/e-mail | `identify` | `POST /auth/identify` | `{login}` | `{exists, displayName}` |
| Senha | `login` | `POST /auth/login` | `{login, password}` | `{twoFactorToken, destination}` |
| 2FA — Verificar | `verifyTwoFactor` | `POST /auth/2fa/verify` | `{login, token, code}` | `{accessToken, refreshToken, expiresIn, user}` |
| 2FA — Reenviar | `resendTwoFactor` | `POST /auth/2fa/resend` | `{login, token}` | `{destination}` |
| Perfil | `handshake` | `POST /auth/handshake` | `{publicKey}` (PEM do cliente) | `{publicKey}` (PEM do servidor) |
| Perfil | `getProfile` | `GET /auth/profile` (Bearer) | — | `{key, data, signature}` (base64) |

Ajuste os caminhos em `integration/auth/AuthIntegration.cpp` e os campos em `models/` conforme a sua API.

## Popup

```cpp
Popup::error(this, "Mensagem");                // atalhos: info / success / warning / error
Popup::display(this, {PopupType::Warning, "Título", "Mensagem", "Sim", "Não"}); // retorna true se confirmou
Popup::setStyle(PopupType::Info, {"ℹ", "Info", QColor("#0ea5e9"), QColor("#f0f9ff")}); // muda a aparência
```
