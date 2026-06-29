$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build'

cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64 `
    -DMINIMIC_MINIMAL_UI=ON `
    -DMINIMIC_ENABLE_ASIO=OFF `
    -DMINIMIC_BUILD_PLUGINS=OFF

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

cmake --build $build --config Release --target SonoBus_Standalone --parallel

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$exe = Join-Path $build 'SonoBus_artefacts\Release\Standalone\MiniMic Link.exe'
Write-Host "Build selesai: $exe"

