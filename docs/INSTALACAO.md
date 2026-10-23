# Instalação

Este guia tem duas partes:

1. [**Instalar o aplicativo**](#1-instalar-o-aplicativo) — para quem só vai usar, a partir de um pacote pronto.
2. [**Preparar o ambiente de desenvolvimento**](#2-ambiente-de-desenvolvimento) — para compilar, depurar e gerar os pacotes.

| Sistema | Pacote para usuários | Pode gerar |
|---|---|---|
| Linux (Ubuntu/Debian) | `.AppImage` ou `.deb` | AppImage e `.deb` |
| Linux (Fedora, Arch, outros) | `.AppImage` | AppImage |
| macOS | `.dmg` | `.dmg` |
| Windows 10/11 | instalador `.exe` ou `.zip` portátil | `.exe` e `.zip` |

> Cada pacote só pode ser gerado no próprio sistema. Para gerar todos de uma vez a partir do Linux, use o
> workflow do GitHub descrito em [Gerar todos os pacotes pelo GitHub](#gerar-todos-os-pacotes-pelo-github).

---

## 1. Instalar o aplicativo

### Linux — AppImage (qualquer distribuição)

O AppImage é um arquivo único que já traz o Qt e o OpenSSL; não precisa instalar nada.

```bash
chmod +x myqtapp-0.1.0-x86_64.AppImage
./myqtapp-0.1.0-x86_64.AppImage
```

Se aparecer o erro `dlopen(): error loading libfuse.so.2`, instale o FUSE 2:

```bash
sudo apt install libfuse2t64      # Ubuntu 24.04 ou mais novo
sudo apt install libfuse2         # Ubuntu 22.04 / Debian 12
sudo dnf install fuse-libs        # Fedora
```

Ou rode sem FUSE: `./myqtapp-0.1.0-x86_64.AppImage --appimage-extract-and-run`.

Para aparecer no menu de aplicativos, use o [Gear Lever](https://flathub.org/apps/it.mijorus.gearlever)
ou o AppImageLauncher, ou mova o arquivo para `~/Applications/`.

**Desinstalar:** apague o arquivo `.AppImage`.

### Linux — .deb (Ubuntu, Debian, Mint, Pop!_OS)

O `.deb` usa o Qt da distribuição; o `apt` instala as dependências sozinho.

```bash
sudo apt install ./myqtapp_0.1.0_amd64.deb
```

O aplicativo aparece no menu como **MyQtApp** e pode ser aberto pelo terminal com `myqtapp`.

> O `.deb` precisa ser gerado numa versão do Ubuntu/Debian igual ou mais antiga que a de destino
> (um pacote feito no Ubuntu 24.04 pode não instalar no 22.04).

**Desinstalar:** `sudo apt remove myqtapp`

### macOS

1. Abra o `myqtapp-0.1.0-Darwin.dmg`.
2. Arraste o **myqtapp** para a pasta **Aplicativos**.
3. Na primeira vez, o macOS bloqueia o app porque ele não é assinado pela Apple:
   - **macOS 15 (Sequoia) ou mais novo:** tente abrir, depois vá em
     *Ajustes do Sistema → Privacidade e Segurança* e clique em **Abrir Mesmo Assim**.
   - **macOS 14 ou anterior:** clique com o botão direito no app → **Abrir** → **Abrir**.
   - Ou, pelo Terminal: `xattr -dr com.apple.quarantine /Applications/myqtapp.app`

O `.dmg` é gerado para a arquitetura do Mac que o criou: num Mac com Apple Silicon (M1/M2/M3/M4) ele
não abre em Macs Intel, e vice-versa.

**Desinstalar:** arraste `/Applications/myqtapp.app` para o Lixo.

### Windows 10/11

**Instalador (`myqtapp-0.1.0-win64.exe`):**

1. Execute o instalador.
2. Se o Windows mostrar *"O Windows protegeu o computador"* (SmartScreen, porque o instalador não é
   assinado), clique em **Mais informações → Executar assim mesmo**.
3. Siga os passos. O app fica em `C:\Program Files\MyQtApp` e aparece no menu Iniciar como **MyQtApp**.

**Portátil (`myqtapp-0.1.0-win64.zip`):** extraia em qualquer pasta e execute `bin\myqtapp.exe`.

**Desinstalar:** *Configurações → Aplicativos → Aplicativos instalados → MyQtApp → Desinstalar*
(ou apague a pasta, na versão portátil).

### Configuração (.env)

Os pacotes já vêm com um `.env` embutido com os endereços dos serviços. Para usar outros endereços sem
gerar o pacote de novo, crie um arquivo `.env` em um destes lugares (o primeiro encontrado vale):

| Sistema | Arquivo |
|---|---|
| AppImage | `.env` na mesma pasta do arquivo `.AppImage` |
| Linux (`.deb` e AppImage) | `~/.config/myqtapp/.env` |
| macOS | `~/Library/Preferences/myqtapp/.env` |
| Windows | `%LOCALAPPDATA%\myqtapp\.env` (ex.: `C:\Users\<você>\AppData\Local\myqtapp\.env`) |

Conteúdo (veja `.env.example`):

```ini
AUTH_API_URL=https://auth.seudominio.com
PERSONAL_API_URL=https://personal.seudominio.com
HTTP_TIMEOUT_MS=15000
```

Variáveis de ambiente com o mesmo nome têm prioridade sobre qualquer `.env`.

### Onde o aplicativo guarda dados

Chaves públicas, salts e arquivos criptografados ficam em:

| Sistema | Pasta |
|---|---|
| Linux | `~/.local/share/myqtapp/` |
| macOS | `~/Library/Application Support/myqtapp/` |
| Windows | `%APPDATA%\myqtapp\` (ex.: `C:\Users\<você>\AppData\Roaming\myqtapp\`) |

Desinstalar o aplicativo **não** apaga essa pasta; apague-a manualmente para remover tudo.

---

## 2. Ambiente de desenvolvimento

Requisitos em todos os sistemas: **CMake 3.16+**, compilador **C++17**, **Qt 6** (Widgets, Network,
Concurrent) e **OpenSSL 1.1.1 ou 3.x**. Para gerar pacotes no Windows e no macOS é preciso **Qt 6.5+**.

### Linux — Ubuntu / Debian

**1. Dependências**

```bash
sudo apt update
sudo apt install cmake build-essential gdb qt6-base-dev libssl-dev

# para gerar os pacotes (AppImage e .deb)
sudo apt install qt6-base-dev-tools qt6-wayland dpkg-dev curl
```

**2. Configurar e rodar**

```bash
cp .env.example .env      # ajuste os endereços se precisar
make run                  # compila (Debug) e executa
make debug                # compila e abre no gdb
```

**3. Gerar os pacotes**

```bash
make package              # AppImage + .deb em dist/
make appimage             # só o AppImage
make deb                  # só o .deb
PACKAGE_MAINTAINER="Seu Nome <voce@email.com>" make deb
```

Na primeira execução o script baixa o `linuxdeploy` (precisa de internet).
O `.deb` sempre usa o Qt do sistema, então não funciona com `QT_PATH`.

### Linux — Fedora

```bash
sudo dnf install cmake gcc-c++ gdb qt6-qtbase-devel openssl-devel
make run
make appimage             # o .deb não se aplica ao Fedora
```

### Linux — Arch / Manjaro

```bash
sudo pacman -S --needed cmake base-devel gdb qt6-base openssl
make run
make appimage
```

### Linux — usando o Qt do instalador oficial

Se preferir uma versão mais nova do Qt (instalada em `~/Qt`), informe o caminho:

```bash
make run QT_PATH=~/Qt/6.8.3/gcc_64
make appimage QT_PATH=~/Qt/6.8.3/gcc_64
```

### macOS

**1. Dependências**

```bash
xcode-select --install                 # compilador (Command Line Tools)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"   # Homebrew, se não tiver
brew install cmake openssl@3
```

Qt — escolha **uma** opção:

- **Instalador oficial** (recomendado para gerar o `.dmg`): baixe em <https://www.qt.io/download-qt-installer>
  e instale o kit **macOS** do Qt 6.5+. O caminho fica como `~/Qt/6.8.3/macos`.
- **Homebrew:** `brew install qt` (o caminho é `$(brew --prefix qt)`).

**2. Configurar e rodar**

```bash
export OPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cp .env.example .env
make run QT_PATH=~/Qt/6.8.3/macos      # ou QT_PATH="$(brew --prefix qt)"
make debug QT_PATH=~/Qt/6.8.3/macos    # abre no lldb
```

Para não repetir, coloque o `export OPENSSL_ROOT_DIR=...` no `~/.zshrc`.

**3. Gerar o .dmg**

```bash
QT_PATH=~/Qt/6.8.3/macos scripts/package.sh
```

O OpenSSL do Homebrew é encontrado sozinho; o `.dmg` sai em `dist/`.

### Windows 10/11

**1. Dependências**

| Programa | Como instalar |
|---|---|
| Visual Studio 2022 (Community serve) | <https://visualstudio.microsoft.com/> — marque **Desenvolvimento para desktop com C++** |
| Qt 6.5+ | <https://www.qt.io/download-qt-installer> — marque o kit **MSVC 2022 64-bit** (ou 2019) |
| OpenSSL | <https://slproweb.com/products/Win32OpenSSL.html> — **Win64 OpenSSL** (versão completa, não a *Light*) |
| CMake | `winget install Kitware.CMake` (ou o que vem com o Visual Studio/Qt) |
| NSIS (só para o instalador `.exe`) | `winget install NSIS.NSIS` |
| Git (opcional) | `winget install Git.Git` |

CMake e NSIS pelo terminal, de uma vez:

```powershell
winget install Kitware.CMake NSIS.NSIS
```

Na instalação do OpenSSL, quando perguntar onde copiar as DLLs, escolha **"The OpenSSL binaries (/bin) directory"**.

**2. Configurar e rodar**

O jeito mais simples é pelo **Qt Creator**: *File → Open File or Project* → `CMakeLists.txt`, escolha
o kit **Desktop Qt 6.x MSVC 64-bit** e clique em **Run** (Ctrl+R). Copie `.env.example` para `.env` antes.

Pelo terminal (PowerShell):

```powershell
Copy-Item .env.example .env
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64 -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
cmake --build build --config Debug
$env:PATH = "C:\Qt\6.8.3\msvc2022_64\bin;C:\Program Files\OpenSSL-Win64\bin;$env:PATH"
.\build\Debug\myqtapp.exe
```

O `PATH` é necessário só para rodar a versão de desenvolvimento (para achar as DLLs do Qt e do OpenSSL);
os pacotes já trazem tudo. Para depurar, abra `build\myqtapp.sln` no Visual Studio, defina **myqtapp**
como projeto de inicialização e pressione F5.

**3. Gerar o instalador .exe e o .zip**

```powershell
scripts\package.cmd
```

ou, com opções:

```powershell
scripts\package.ps1 -QtPath C:\Qt\6.8.3\msvc2022_64 -Targets exe -Clean
Get-Help scripts\package.ps1 -Detailed
```

O script acha sozinho o Qt mais novo em `C:\Qt` e o OpenSSL em `C:\Program Files\OpenSSL-Win64`.
Os arquivos saem em `dist\`. Também dá para usar o Git Bash: `QT_PATH=C:/Qt/6.8.3/msvc2022_64 scripts/package.sh`.

### Gerar todos os pacotes pelo GitHub

O workflow `.github/workflows/packages.yml` compila em máquinas Linux, macOS e Windows do GitHub:

1. Suba o projeto para um repositório no GitHub.
2. Vá em **Actions → Pacotes → Run workflow** (ou crie uma tag: `git tag v0.1.0 && git push --tags`).
3. Ao terminar, baixe os arquivos em **Artifacts** (`linux`, `macos`, `windows`).

Como o `.env` não vai para o repositório (está no `.gitignore`), os pacotes gerados lá levam o
`.env.example`. Ajuste-o com os endereços reais ou configure o `.env` depois da instalação.
Para o mantenedor do `.deb`, crie a variável `PACKAGE_MAINTAINER` em
*Settings → Secrets and variables → Actions → Variables*.

---

## Problemas comuns

| Sintoma | Solução |
|---|---|
| `make: cmake: Arquivo ou diretório inexistente` | `sudo apt install cmake build-essential` |
| `Could NOT find OpenSSL` | Linux: `sudo apt install libssl-dev`. macOS: `export OPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"`. Windows: instale o Win64 OpenSSL completo ou passe `-OpenSslRoot` |
| `Could not find a package configuration file provided by "Qt6"` | Instale o Qt 6 ou informe o caminho: `QT_PATH=...` (Linux/macOS) ou `-QtPath ...` (Windows) |
| GNOME: `GLib-GIO-ERROR ... antialiasing` e o app fecha | Já tratado no código (`QT_QPA_PLATFORMTHEME=gnome`). Se definiu essa variável manualmente com `gtk3`, remova-a |
| `build-appimage.sh: Bad substitution` | O script antigo foi substituído; use `make appimage` ou `scripts/package.sh` |
| Windows: `não pode ser carregado porque a execução de scripts foi desabilitada` | Use `scripts\package.cmd` ou `powershell -ExecutionPolicy Bypass -File scripts\package.ps1` |
| Windows: `libcrypto-3-x64.dll não foi encontrado` ao rodar pelo `build\` | Adicione `C:\Program Files\OpenSSL-Win64\bin` ao `PATH` (veja o passo 2 do Windows) |
| Windows: `NSIS não encontrado` | `winget install NSIS.NSIS` e abra um terminal novo |
| macOS: *"myqtapp está danificado e não pode ser aberto"* | `xattr -dr com.apple.quarantine /Applications/myqtapp.app` |
| AppImage: `libfuse.so.2` | Veja [Linux — AppImage](#linux--appimage-qualquer-distribuição) |
| `Env: arquivo .env não encontrado` no terminal | Crie o `.env` (veja [Configuração](#configuração-env)); sem ele os serviços ficam sem endereço |
