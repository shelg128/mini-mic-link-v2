$ErrorActionPreference = 'Stop'

$sdk = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } else { $env:ANDROID_HOME }
if (-not $sdk -or -not (Test-Path -LiteralPath $sdk)) {
    throw 'Android SDK tidak ditemukan. Set ANDROID_SDK_ROOT atau ANDROID_HOME terlebih dahulu.'
}

if (-not (Get-Command java -ErrorAction SilentlyContinue)) {
    throw 'Java/JDK tidak ditemukan di PATH.'
}

$androidProject = Join-Path (Split-Path -Parent $PSScriptRoot) 'mobile\Builds\Android'
$assetSource = Join-Path (Split-Path -Parent $PSScriptRoot) 'images\android'
$resourceTarget = Join-Path $androidProject 'app\src\main\res'

New-Item -ItemType Directory -Path $resourceTarget -Force | Out-Null
Get-ChildItem -LiteralPath $assetSource -Directory | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $resourceTarget -Recurse -Force
}

Push-Location $androidProject
try {
    & .\gradlew.bat assembleDebug
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}

$root = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $root 'dist'
New-Item -ItemType Directory -Path $dist -Force | Out-Null

$version = '0.1.0'
$abiMap = @{
    'arm64-v8a' = 'arm64-v8a'
    'armeabi-v7a' = 'armeabi-v7a'
    'x86' = 'x86'
    'x86_64' = 'x86_64'
}

$apkRoot = Join-Path $androidProject 'app\build\outputs\apk'
$splitApks = Get-ChildItem -LiteralPath $apkRoot -Recurse -Filter '*.apk' |
    Where-Object { $_.Name -match 'debug' -and $_.Name -notmatch 'universal' }

foreach ($abi in $abiMap.Keys) {
    $apk = $splitApks | Where-Object { $_.Name -match [regex]::Escape($abi) } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $apk) {
        throw "APK ABI $abi tidak ditemukan di $apkRoot"
    }

    $target = Join-Path $dist "MiniMic-Link-Android-$abi-$version-debug.apk"
    Copy-Item -LiteralPath $apk.FullName -Destination $target -Force
}

Write-Host "APK split debug tersedia di $dist"
Get-ChildItem -LiteralPath $dist -Filter "MiniMic-Link-Android-*-$version-debug.apk" |
    Sort-Object Name |
    Select-Object Name, Length, LastWriteTime |
    Format-Table -AutoSize
