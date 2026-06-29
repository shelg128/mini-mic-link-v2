$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$iscc = @(
    'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
    'C:\Program Files\Inno Setup 6\ISCC.exe'
) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if (-not $iscc) {
    throw 'Inno Setup 6 tidak ditemukan. Install Inno Setup atau build portable ZIP saja.'
}

$exe = Join-Path $root 'build\SonoBus_artefacts\Release\Standalone\MiniMic Link.exe'
if (-not (Test-Path -LiteralPath $exe)) {
    throw "MiniMic Link.exe belum ada. Jalankan scripts\build-minimic-windows.ps1 dulu."
}

$dist = Join-Path $root 'dist'
New-Item -ItemType Directory -Path $dist -Force | Out-Null

$script = Join-Path $root 'installer\windows\MiniMicLink.iss'
& $iscc $script

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Installer tersedia di $dist"
