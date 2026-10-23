<#
.SYNOPSIS
    Gera os pacotes do Windows em dist\: instalador .exe (NSIS) e .zip portátil.

.DESCRIPTION
    Compila em Release, copia Qt e OpenSSL para o pacote (windeployqt, via CMake) e
    roda o CPack. Equivale a "scripts/package.sh exe", sem precisar do Git Bash.

    Requisitos:
      - Qt 6.5+ (MSVC) pelo instalador oficial
      - Visual Studio 2019/2022 com "Desenvolvimento para desktop com C++"
      - CMake (vem com o Visual Studio e com o Qt)
      - OpenSSL: https://slproweb.com/products/Win32OpenSSL.html (Win64, versão completa)
      - NSIS (instalador .exe): winget install NSIS.NSIS

.PARAMETER QtPath
    Pasta do Qt, ex.: C:\Qt\6.8.3\msvc2022_64. Se omitido, usa $env:QT_PATH ou
    procura a versão mais nova em C:\Qt.

.PARAMETER OpenSslRoot
    Pasta do OpenSSL. Se omitido, usa $env:OPENSSL_ROOT_DIR ou procura em
    "C:\Program Files\OpenSSL-Win64" e "C:\Program Files\OpenSSL".

.PARAMETER Targets
    exe, zip ou ambos (padrão).

.PARAMETER EnvFile
    .env embutido no pacote (padrão: .env, ou .env.example).

.PARAMETER Generator
    Gerador do CMake. Padrão: Visual Studio mais novo (MSVC) ou Ninja/MinGW (Qt MinGW).

.PARAMETER Clean
    Apaga a pasta de build antes de compilar.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\package.ps1

.EXAMPLE
    scripts\package.ps1 -QtPath C:\Qt\6.8.3\msvc2022_64 -Targets exe
#>
[CmdletBinding()]
param(
    [string]$QtPath = $env:QT_PATH,
    [string]$OpenSslRoot = $env:OPENSSL_ROOT_DIR,
    [ValidateSet('exe', 'zip')]
    [string[]]$Targets = @('exe', 'zip'),
    [string]$EnvFile = $env:ENV_FILE,
    [string]$Generator,
    [int]$Jobs = [Environment]::ProcessorCount,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root 'build-package\windows'
$DistDir = Join-Path $Root 'dist'

function Write-Step([string]$Message) { Write-Host "==> $Message" -ForegroundColor Cyan }
function Write-Warn([string]$Message) { Write-Host "Aviso: $Message" -ForegroundColor Yellow }
function Stop-WithError([string]$Message) {
    Write-Host "Erro: $Message" -ForegroundColor Red
    exit 1
}

# Executa um programa e para o script se ele falhar (o PowerShell 5.1 não faz isso sozinho)
function Invoke-Native([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        Stop-WithError "$([IO.Path]::GetFileName($Program)) terminou com código $LASTEXITCODE"
    }
}

# ---------------------------------------------------------------- Qt
function Find-Qt {
    $qtRoot = 'C:\Qt'
    if (-not (Test-Path $qtRoot)) { return $null }

    $versions = Get-ChildItem $qtRoot -Directory |
        Where-Object { $_.Name -match '^6\.\d+(\.\d+)?$' } |
        Sort-Object { [version]$_.Name } -Descending

    foreach ($version in $versions) {
        # prefere MSVC (o OpenSSL para Windows é compilado com MSVC)
        foreach ($pattern in @('msvc*_64', 'mingw*_64')) {
            $kit = Get-ChildItem $version.FullName -Directory -Filter $pattern |
                Sort-Object Name -Descending | Select-Object -First 1
            if ($kit -and (Test-Path (Join-Path $kit.FullName 'lib\cmake\Qt6'))) {
                return $kit.FullName
            }
        }
    }
    return $null
}

if (-not $QtPath) { $QtPath = Find-Qt }
if (-not $QtPath -or -not (Test-Path (Join-Path $QtPath 'lib\cmake\Qt6'))) {
    Stop-WithError ("Qt 6 não encontrado. Informe a pasta do kit, ex.: " +
        "-QtPath C:\Qt\6.8.3\msvc2022_64 (instale o Qt 6.5+ pelo instalador oficial)")
}
$QtPath = (Resolve-Path $QtPath).Path
$IsMinGW = (Split-Path $QtPath -Leaf) -like 'mingw*'

$qmake = Join-Path $QtPath 'bin\qmake.exe'
if (Test-Path $qmake) {
    $qtVersion = (& $qmake -query QT_VERSION).Trim()
    Write-Step "Qt $qtVersion ($QtPath)"
    if ([version]$qtVersion -lt [version]'6.5') {
        Stop-WithError "o empacotamento no Windows requer Qt 6.5 ou mais novo"
    }
} else {
    Write-Step "Qt ($QtPath)"
}

# Qt MinGW: o compilador fica em C:\Qt\Tools\mingw*_64\bin
if ($IsMinGW) {
    $qtInstallRoot = Split-Path (Split-Path $QtPath -Parent) -Parent
    $mingw = Get-ChildItem (Join-Path $qtInstallRoot 'Tools') -Directory -Filter 'mingw*_64' -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1
    if ($mingw) {
        $env:PATH = "$(Join-Path $mingw.FullName 'bin');$env:PATH"
    } elseif (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
        Stop-WithError "compilador MinGW não encontrado (instale o MinGW pelo Qt Maintenance Tool)"
    }
    Write-Warn "Qt MinGW: o OpenSSL precisa ter bibliotecas compatíveis com MinGW; o recomendado é o Qt MSVC"
}

# ---------------------------------------------------------------- ferramentas
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    # CMake que acompanha o Qt
    $bundled = Join-Path (Split-Path (Split-Path $QtPath -Parent) -Parent) 'Tools\CMake_64\bin'
    if (Test-Path (Join-Path $bundled 'cmake.exe')) {
        $env:PATH = "$bundled;$env:PATH"
    } else {
        Stop-WithError "cmake não encontrado. Instale com: winget install Kitware.CMake"
    }
}

if ($Targets -contains 'exe') {
    if (-not (Get-Command makensis -ErrorAction SilentlyContinue)) {
        $nsis = Join-Path ${env:ProgramFiles(x86)} 'NSIS'
        if (Test-Path (Join-Path $nsis 'makensis.exe')) {
            $env:PATH = "$nsis;$env:PATH"
        } else {
            Stop-WithError "NSIS não encontrado (gera o instalador .exe). Instale com: winget install NSIS.NSIS"
        }
    }
}

# ---------------------------------------------------------------- OpenSSL
if (-not $OpenSslRoot) {
    foreach ($candidate in @("$env:ProgramFiles\OpenSSL-Win64", "$env:ProgramFiles\OpenSSL")) {
        if (Test-Path (Join-Path $candidate 'include\openssl')) { $OpenSslRoot = $candidate; break }
    }
}
if ($OpenSslRoot) {
    Write-Step "OpenSSL ($OpenSslRoot)"
} else {
    Write-Warn "OpenSSL não encontrado nos lugares padrão; o CMake vai procurar sozinho (instale o Win64 OpenSSL de slproweb.com ou informe -OpenSslRoot)"
}

# ---------------------------------------------------------------- compilação
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Step 'Limpando a pasta de build'
    Remove-Item $BuildDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $BuildDir, $DistDir | Out-Null

$configureArgs = @('-S', $Root, '-B', $BuildDir, '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_PREFIX_PATH=$QtPath")
if ($OpenSslRoot) { $configureArgs += "-DOPENSSL_ROOT_DIR=$OpenSslRoot" }
if ($EnvFile) {
    if (-not (Test-Path $EnvFile)) { Stop-WithError "arquivo .env não encontrado: $EnvFile" }
    $configureArgs += "-DAPP_ENV_FILE=$((Resolve-Path $EnvFile).Path)"
}

# o gerador não pode mudar numa pasta já configurada
$alreadyConfigured = Test-Path (Join-Path $BuildDir 'CMakeCache.txt')
if (-not $Generator -and -not $alreadyConfigured -and $IsMinGW) {
    $Generator = if (Get-Command ninja -ErrorAction SilentlyContinue) { 'Ninja' } else { 'MinGW Makefiles' }
}
if ($Generator) {
    $configureArgs += @('-G', $Generator)
    if ($Generator -like 'Visual Studio*') { $configureArgs += @('-A', 'x64') }
} elseif (-not $alreadyConfigured -and -not $env:CMAKE_GENERATOR) {
    $configureArgs += @('-A', 'x64') # gerador padrão: Visual Studio mais novo instalado
}

Write-Step 'Configurando'
Invoke-Native 'cmake' $configureArgs

Write-Step 'Compilando (Release)'
Invoke-Native 'cmake' @('--build', $BuildDir, '--config', 'Release', '--parallel', "$Jobs")

# ---------------------------------------------------------------- pacotes
$cpackDir = Join-Path $BuildDir 'cpack'
if (Test-Path $cpackDir) { Remove-Item $cpackDir -Recurse -Force }

$generators = @()
if ($Targets -contains 'exe') { $generators += 'NSIS' }
if ($Targets -contains 'zip') { $generators += 'ZIP' }

Push-Location $BuildDir
try {
    foreach ($cpackGenerator in $generators) {
        Write-Step "Gerando pacote ($cpackGenerator)"
        Invoke-Native 'cpack' @('-G', $cpackGenerator, '-C', 'Release', '-B', $cpackDir)
    }
} finally {
    Pop-Location
}

$packages = Get-ChildItem $cpackDir -File | Where-Object { $_.Extension -in '.exe', '.zip' }
if (-not $packages) { Stop-WithError 'o CPack não gerou nenhum pacote' }

foreach ($package in $packages) {
    Move-Item $package.FullName (Join-Path $DistDir $package.Name) -Force
}

Write-Step 'Pacotes em dist\:'
$packages | ForEach-Object { Write-Host "    $($_.Name)" }
