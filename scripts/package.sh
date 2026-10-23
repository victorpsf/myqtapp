#!/usr/bin/env bash
# Gera os pacotes de distribuição em dist/.
#
#   Linux   -> AppImage e .deb
#   macOS   -> .dmg (com o myqtapp.app dentro)
#   Windows -> instalador .exe e .zip portátil (rodar no Git Bash)
#
# Cada formato só pode ser gerado no próprio sistema: o .exe precisa do Qt/MSVC do Windows
# e o .dmg precisa do macdeployqt/SDK da Apple. Para gerar todos a partir do Linux,
# use o workflow .github/workflows/packages.yml.
#
# Uso:
#   scripts/package.sh                  todos os formatos do sistema atual
#   scripts/package.sh appimage         só o AppImage   (Linux)
#   scripts/package.sh deb              só o .deb       (Linux)
#   scripts/package.sh dmg              só o .dmg       (macOS)
#   scripts/package.sh exe              .exe e .zip     (Windows)
#
# Variáveis opcionais:
#   QT_PATH=~/Qt/6.8.0/gcc_64    Qt instalado pelo instalador oficial (obrigatório no Windows/Mac)
#   OPENSSL_ROOT_DIR=...         OpenSSL fora do lugar padrão (Windows/Mac)
#   ENV_FILE=caminho/.env        .env embutido nos pacotes (padrão: .env, ou .env.example)
#   PACKAGE_MAINTAINER="Nome <email>"   mantenedor exibido no .deb
#   JOBS=8                       compilações em paralelo

# O script usa recursos do bash; se foi chamado com `sh` (dash no Ubuntu), reabre no bash.
if [ -z "${BASH_VERSION:-}" ]; then
    exec bash "$0" "$@"
fi

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$ROOT/build-package"
DIST_DIR="$ROOT/dist"

APP_NAME="myqtapp"
ARCH="$(uname -m)"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
QT_PATH="${QT_PATH:-}"
ENV_FILE="${ENV_FILE:-}"
PACKAGE_MAINTAINER="${PACKAGE_MAINTAINER:-}"

info() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33mAviso:\033[0m %s\n' "$*" >&2; }
fail() { printf '\033[1;31mErro:\033[0m %s\n' "$*" >&2; exit 1; }

case "$(uname -s)" in
    Linux*)               PLATFORM=linux ;;
    Darwin*)              PLATFORM=macos ;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM=windows ;;
    *)                    fail "sistema não suportado: $(uname -s)" ;;
esac

# ---------------------------------------------------------------- alvos
supported_targets() {
    case "$PLATFORM" in
        linux)   echo "appimage deb" ;;
        macos)   echo "dmg" ;;
        windows) echo "exe" ;;
    esac
}

TARGETS=("$@")
[[ ${#TARGETS[@]} -gt 0 ]] || read -r -a TARGETS <<< "$(supported_targets)"

for target in "${TARGETS[@]}"; do
    case "$target" in
        appimage|deb) [[ "$PLATFORM" == linux ]]   || fail "$target só pode ser gerado no Linux" ;;
        dmg)          [[ "$PLATFORM" == macos ]]   || fail ".dmg só pode ser gerado no macOS" ;;
        exe)          [[ "$PLATFORM" == windows ]] || fail ".exe só pode ser gerado no Windows (Git Bash)" ;;
        *)            fail "alvo desconhecido: $target (use: appimage, deb, dmg, exe)" ;;
    esac
done

# ---------------------------------------------------------------- dependências
command -v cmake >/dev/null 2>&1 || case "$PLATFORM" in
    linux)   fail "cmake não encontrado. Instale com: sudo apt install cmake build-essential qt6-base-dev libssl-dev" ;;
    macos)   fail "cmake não encontrado. Instale com: brew install cmake openssl@3" ;;
    windows) fail "cmake não encontrado. Instale o CMake (vem com o Qt Creator/Visual Studio) e coloque no PATH" ;;
esac

if [[ "$PLATFORM" != linux && -z "$QT_PATH" ]]; then
    fail "informe o Qt 6.5+ instalado, ex.: QT_PATH=~/Qt/6.8.0/macos ou QT_PATH=C:/Qt/6.8.0/msvc2022_64"
fi

if [[ "$PLATFORM" == macos && -z "${OPENSSL_ROOT_DIR:-}" ]] && command -v brew >/dev/null 2>&1; then
    OPENSSL_ROOT_DIR="$(brew --prefix openssl@3 2>/dev/null || true)"
fi

mkdir -p "$WORK_DIR" "$DIST_DIR"

# ---------------------------------------------------------------- compilação
# Compila em Release (prefixo /usr no Linux; no Windows/Mac o CPack define o destino)
configure_and_build() {
    local build_dir="$1"
    local args=(-DCMAKE_BUILD_TYPE=Release)
    [[ "$PLATFORM" == linux ]]           && args+=(-DCMAKE_INSTALL_PREFIX=/usr)
    [[ -n "$QT_PATH" ]]                  && args+=("-DCMAKE_PREFIX_PATH=$QT_PATH")
    [[ -n "${OPENSSL_ROOT_DIR:-}" ]]     && args+=("-DOPENSSL_ROOT_DIR=$OPENSSL_ROOT_DIR")
    [[ -n "$ENV_FILE" ]]                 && args+=("-DAPP_ENV_FILE=$(cd "$(dirname "$ENV_FILE")" && pwd)/$(basename "$ENV_FILE")")
    [[ -n "$PACKAGE_MAINTAINER" ]]       && args+=("-DPACKAGE_MAINTAINER=$PACKAGE_MAINTAINER")

    info "Compilando (Release)"
    cmake -S "$ROOT" -B "$build_dir" "${args[@]}"
    cmake --build "$build_dir" --config Release -j "$JOBS"
}

# Roda o CPack e move os pacotes gerados para dist/
run_cpack() {
    local build_dir="$1"; shift
    local out_dir="$build_dir/cpack"
    rm -rf "$out_dir"
    for generator in "$@"; do
        info "Gerando pacote ($generator)"
        (cd "$build_dir" && cpack -G "$generator" -C Release -B "$out_dir")
    done
    find "$out_dir" -maxdepth 1 -type f \( -name '*.deb' -o -name '*.dmg' -o -name '*.exe' -o -name '*.zip' \) \
        -exec mv -f {} "$DIST_DIR/" \; -exec printf '    %s\n' {} \;
}

VERSION="$(sed -nE 's/^project\([^)]*VERSION[[:space:]]+([0-9.]+).*/\1/p' "$ROOT/CMakeLists.txt" | head -n1)"
VERSION="${VERSION:-0.0.0}"

# ---------------------------------------------------------------- AppImage
download() {
    if command -v curl >/dev/null 2>&1; then
        curl -fL --progress-bar -o "$2" "$1"
    elif command -v wget >/dev/null 2>&1; then
        wget -q --show-progress -O "$2" "$1"
    else
        fail "curl ou wget é necessário para baixar o linuxdeploy (sudo apt install curl)"
    fi
}

# qmake é usado pelo plugin do Qt para descobrir onde estão bibliotecas e plugins
find_qmake() {
    local candidates=()
    [[ -n "$QT_PATH" ]] && candidates+=("$QT_PATH/bin/qmake6" "$QT_PATH/bin/qmake")
    candidates+=("$(command -v qmake6 || true)" /usr/lib/qt6/bin/qmake
                 "/usr/lib/$ARCH-linux-gnu/qt6/bin/qmake" "$(command -v qmake || true)")
    for candidate in "${candidates[@]}"; do
        [[ -n "$candidate" && -x "$candidate" ]] || continue
        if "$candidate" -query QT_VERSION 2>/dev/null | grep -q '^6\.'; then
            echo "$candidate"
            return 0
        fi
    done
    return 1
}

build_appimage() {
    local build_dir="$WORK_DIR/appimage"
    local appdir="$build_dir/AppDir"
    local tools_dir="$WORK_DIR/tools"
    local output_file="$DIST_DIR/$APP_NAME-$VERSION-$ARCH.AppImage"

    command -v g++ >/dev/null 2>&1 || fail "g++ não encontrado. Instale com: sudo apt install build-essential"

    local qmake="${QMAKE:-$(find_qmake || true)}"
    [[ -n "$qmake" ]] || fail "qmake do Qt 6 não encontrado. Instale com: sudo apt install qmake6 (ou qt6-base-dev-tools), ou informe QT_PATH=/caminho/do/qt"
    local qt_libs qt_plugins
    qt_libs="$("$qmake" -query QT_INSTALL_LIBS)"
    qt_plugins="$("$qmake" -query QT_INSTALL_PLUGINS)"
    info "AppImage — Qt $("$qmake" -query QT_VERSION) ($qmake)"

    # linuxdeploy + plugin do Qt (copiam Qt, OpenSSL e demais bibliotecas para o AppDir)
    mkdir -p "$tools_dir"
    local linuxdeploy="$tools_dir/linuxdeploy-$ARCH.AppImage"
    local linuxdeploy_qt="$tools_dir/linuxdeploy-plugin-qt-$ARCH.AppImage"
    local releases="https://github.com/linuxdeploy"
    if [[ ! -x "$linuxdeploy" ]]; then
        info "Baixando linuxdeploy"
        download "$releases/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage" "$linuxdeploy"
        chmod +x "$linuxdeploy"
    fi
    if [[ ! -x "$linuxdeploy_qt" ]]; then
        info "Baixando linuxdeploy-plugin-qt"
        download "$releases/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage" "$linuxdeploy_qt"
        chmod +x "$linuxdeploy_qt"
    fi

    configure_and_build "$build_dir"

    info "Montando o AppDir"
    rm -rf "$appdir"
    DESTDIR="$appdir" cmake --install "$build_dir"

    # Wayland (GNOME): inclui o plugin se o Qt instalado tiver (pacote qt6-wayland)
    local extra_platforms=""
    if [[ -f "$qt_plugins/platforms/libqwayland-generic.so" ]]; then
        extra_platforms="libqwayland-egl.so;libqwayland-generic.so"
        info "Incluindo suporte a Wayland"
    fi

    info "Gerando o AppImage"
    rm -f "$output_file"
    (
        cd "$build_dir"
        export QMAKE="$qmake"
        export PATH="$tools_dir:$PATH"            # o linuxdeploy procura o plugin no PATH
        export LD_LIBRARY_PATH="$qt_libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
        export APPIMAGE_EXTRACT_AND_RUN=1         # dispensa FUSE para rodar as ferramentas
        export LDAI_OUTPUT="$output_file"         # linuxdeploy-plugin-appimage (versões novas)
        export OUTPUT="$output_file"              # linuxdeploy-plugin-appimage (versões antigas)
        export VERSION
        [[ -n "$extra_platforms" ]] && export EXTRA_PLATFORM_PLUGINS="$extra_platforms"

        "$linuxdeploy" \
            --appdir "$appdir" \
            --executable "$appdir/usr/bin/$APP_NAME" \
            --desktop-file "$appdir/usr/share/applications/$APP_NAME.desktop" \
            --icon-file "$appdir/usr/share/icons/hicolor/scalable/apps/$APP_NAME.svg" \
            --plugin qt \
            --output appimage
    )

    # versões antigas do plugin ignoram OUTPUT/LDAI_OUTPUT e gravam no diretório atual
    if [[ ! -f "$output_file" ]]; then
        local generated
        generated="$(ls -t "$build_dir"/*.AppImage 2>/dev/null | head -n1 || true)"
        [[ -n "$generated" ]] || fail "o linuxdeploy não gerou o AppImage"
        mv "$generated" "$output_file"
    fi
    chmod +x "$output_file"
    printf '    %s\n' "$output_file"
}

# ---------------------------------------------------------------- .deb
build_deb() {
    command -v dpkg-shlibdeps >/dev/null 2>&1 \
        || fail "dpkg-shlibdeps não encontrado. Instale com: sudo apt install dpkg-dev"
    if [[ -n "$QT_PATH" ]]; then
        # o .deb depende dos pacotes Qt da distribuição; um Qt de ~/Qt não seria encontrado no destino
        fail "o .deb usa o Qt do sistema (qt6-base-dev). Rode sem QT_PATH: scripts/package.sh deb"
    fi
    [[ -n "$PACKAGE_MAINTAINER" ]] \
        || warn "PACKAGE_MAINTAINER não definido; o .deb usará \"MyQtApp <maintainer@example.com>\""

    local build_dir="$WORK_DIR/deb"
    configure_and_build "$build_dir"
    run_cpack "$build_dir" DEB
}

# ---------------------------------------------------------------- .dmg
build_dmg() {
    local build_dir="$WORK_DIR/macos"
    configure_and_build "$build_dir"
    run_cpack "$build_dir" DragNDrop
}

# ---------------------------------------------------------------- .exe
build_exe() {
    command -v makensis >/dev/null 2>&1 \
        || [[ -x "/c/Program Files (x86)/NSIS/makensis.exe" ]] \
        || fail "NSIS não encontrado (gera o instalador .exe). Instale com: winget install NSIS.NSIS"
    export PATH="/c/Program Files (x86)/NSIS:$PATH"

    local build_dir="$WORK_DIR/windows"
    configure_and_build "$build_dir"
    run_cpack "$build_dir" NSIS ZIP
}

# ---------------------------------------------------------------- execução
for target in "${TARGETS[@]}"; do
    "build_$target"
done

info "Pacotes em ${DIST_DIR#"$ROOT"/}/:"
ls -1 "$DIST_DIR"
