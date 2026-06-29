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

Write-Host "APK debug tersedia di $androidProject\app\build\outputs\apk"
