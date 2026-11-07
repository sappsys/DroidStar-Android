# Build a self-contained Windows x64 folder under dist/.
# Run on Windows with Qt 6.6+ (MSVC or MinGW). Not for cross-compile from Linux.
param(
    [string]$QtDir = $env:QT_DIR
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

if (-not $QtDir) {
    $candidates = @(
        "C:\Qt\6.6.3\msvc2019_64",
        "C:\Qt\6.6.3\msvc2022_64",
        "C:\Qt\6.6.3\mingw_64"
    )
    foreach ($c in $candidates) {
        if (Test-Path (Join-Path $c "bin\qmake.exe")) {
            $QtDir = $c
            break
        }
    }
}
if (-not $QtDir -or -not (Test-Path (Join-Path $QtDir "bin\qmake.exe"))) {
    Write-Error "Set QT_DIR to your Qt 6.6 x64 kit (e.g. C:\Qt\6.6.3\msvc2019_64)"
}

$qmake = Join-Path $QtDir "bin\qmake.exe"
$windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
$build = Join-Path $root "build\windows"
New-Item -ItemType Directory -Force -Path $build | Out-Null
Set-Location $build

& $qmake (Join-Path $root "DroidStar.pro") "CONFIG+=release"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$nmake = Get-Command nmake -ErrorAction SilentlyContinue
$mingwMake = Get-Command mingw32-make -ErrorAction SilentlyContinue
if ($nmake) {
    & nmake
} elseif ($mingwMake) {
    & mingw32-make -j$env:NUMBER_OF_PROCESSORS
} else {
    Write-Error "Neither nmake nor mingw32-make found in PATH"
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Get-ChildItem -Path $build -Filter "DroidStar.exe" -Recurse | Select-Object -First 1
if (-not $exe) {
    Write-Error "DroidStar.exe not found under $build"
}

$verLine = Select-String -Path (Join-Path $root "DroidStar.pro") -Pattern '^win32:DEFINES \+= APP_VERSION' | Select-Object -First 1
$ver = "0.45.07"
if ($verLine -match 'APP_VERSION=\\"([^\\]+)\\"') {
    $ver = $Matches[1]
}

$dist = Join-Path $root "dist\DroidStar-$ver-windows-x64"
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item $exe.FullName (Join-Path $dist "DroidStar.exe")

& $windeployqt --release --qmldir (Join-Path $root "ui2026") --compiler-runtime (Join-Path $dist "DroidStar.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Neural TX clarity (sherpa-onnx / onnxruntime). Linked at build time; ship beside the EXE.
$sherpaLib = Join-Path $root "third_party\sherpa-onnx\lib\win-x64"
foreach ($dll in @(
    "sherpa-onnx-c-api.dll",
    "onnxruntime.dll",
    "onnxruntime_providers_shared.dll"
)) {
    $src = Join-Path $sherpaLib $dll
    if (-not (Test-Path $src)) {
        Write-Error "Missing $src (Neural AI runtime)"
    }
    Copy-Item $src (Join-Path $dist $dll) -Force
}

Write-Host "Windows package: $dist"
