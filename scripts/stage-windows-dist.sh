#!/bin/sh
# Copy the freshly built Windows EXE into dist/DroidStar-<ver>-windows-x64/
# (the package of record), then rebuild the .zip from that directory.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
# DroidStar.pro uses win32:DEFINES += APP_VERSION=\\\"0.45.07c\\\"
ver=$(sed -n 's/^win32:DEFINES += APP_VERSION=\\"\\?"*\([0-9a-zA-Z.]*\)\\"\\?"*/\1/p' "$root/DroidStar.pro" | head -1)
if [ -z "$ver" ]; then
    ver=$(grep -m1 '^win32:DEFINES += APP_VERSION=' "$root/DroidStar.pro" | sed 's/.*APP_VERSION=//; s/\\//g; s/"//g; s/ //g')
fi
: "${ver:?win32 APP_VERSION not found in DroidStar.pro}"

exe=$root/build/windows/release/DroidStar.exe
dist=$root/dist/DroidStar-$ver-windows-x64
zip=$root/dist/DroidStar-$ver-windows-x64.zip

if [ ! -f "$exe" ]; then
    echo "Missing $exe — build Windows first." >&2
    exit 1
fi
if [ ! -d "$dist" ]; then
    echo "Missing $dist — run a full windeployqt package once before staging." >&2
    exit 1
fi

cp -f "$exe" "$dist/DroidStar.exe"
rm -f "$zip"
( cd "$root/dist" && zip -qr "DroidStar-$ver-windows-x64.zip" "DroidStar-$ver-windows-x64" )

echo "Staged: $dist/DroidStar.exe"
echo "Zip:    $zip"
sha256sum "$dist/DroidStar.exe"
