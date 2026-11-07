#!/bin/sh
# Build the arm64 release APK. Requires Qt 6.6.3 Android, NDK 25.1, JDK 17,
# and android/release/signing.properties plus the release keystore.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
: "${QT_ANDROID:=/opt/Qt/6.6.3/android_arm64_v8a}"
: "${QT_HOST:=/opt/Qt/6.6.3/gcc_64}"
: "${ANDROID_SDK_ROOT:=/opt/android-sdk}"
: "${ANDROID_NDK_ROOT:=$ANDROID_SDK_ROOT/ndk/25.1.8937393}"
: "${JAVA_HOME:=/usr/lib/jvm/java-17-openjdk-amd64}"
export ANDROID_SDK_ROOT ANDROID_NDK_ROOT JAVA_HOME
export PATH="$JAVA_HOME/bin:$ANDROID_SDK_ROOT/platform-tools:$PATH"
androiddeployqt=$QT_HOST/bin/androiddeployqt
if [ ! -x "$androiddeployqt" ]; then
    echo "Missing androiddeployqt: $androiddeployqt" >&2
    exit 1
fi

signing_props=$root/android/release/signing.properties
if [ ! -f "$signing_props" ]; then
    echo "Missing $signing_props" >&2
    exit 1
fi
# shellcheck disable=SC1090
. "$signing_props"
: "${storeFile:?storeFile missing in signing.properties}"
: "${storePassword:?storePassword missing in signing.properties}"
: "${keyAlias:?keyAlias missing in signing.properties}"
: "${keyPassword:?keyPassword missing in signing.properties}"
case $storeFile in
    /*) keystore=$storeFile ;;
    *) keystore=$root/android/release/$storeFile ;;
esac
if [ ! -f "$keystore" ]; then
    echo "Missing keystore: $keystore" >&2
    exit 1
fi

build=$root/build/android
mkdir -p "$build"
cd "$build"
"$QT_ANDROID/bin/qmake" "$root/DroidStar.pro" -spec android-clang CONFIG+=release

# qmake puts APP_VERSION into DEFINES, but make will not rebuild .o files when only
# those defines change. Invalidate objects when the Android letter version moves so
# About / get_software_build() cannot keep showing the previous letter.
ver=$(sed -n 's/^android:ANDROID_VERSION_NAME = //p' "$root/DroidStar.pro" | head -1 | tr -d '[:space:]')
stamp=$build/.app-version-stamp
if [ ! -f "$stamp" ] || [ "$(cat "$stamp")" != "$ver" ]; then
    echo "Version stamp $ver: rebuilding objects so APP_VERSION matches."
    find "$build" -maxdepth 1 \( -name '*.o' -o -name 'moc_*.cpp' -o -name 'libDroidStar_arm64-v8a.so' \) -delete
    printf '%s\n' "$ver" > "$stamp"
fi

make -j"$(nproc)"

# Gradle can package a stale copy of libDroidStar. Force the library that was
# just built into the APK tree before androiddeployqt packages it.
built_lib=$build/libDroidStar_arm64-v8a.so
apk_lib_dir=$build/android-build/libs/arm64-v8a
if [ ! -f "$built_lib" ]; then
    echo "Missing built library: $built_lib" >&2
    exit 1
fi
rm -rf "$build/android-build/build/intermediates/merged_native_libs" \
       "$build/android-build/build/intermediates/stripped_native_libs" \
       "$build/android-build/build/intermediates/dex" \
       "$build/android-build/build/intermediates/javac" 2>/dev/null || true
mkdir -p "$apk_lib_dir"
cp -f "$built_lib" "$apk_lib_dir/libDroidStar_arm64-v8a.so"
lib_hash=$(sha256sum "$built_lib" | awk '{print $1}')

"$androiddeployqt" \
    --input "$build/android-DroidStar-deployment-settings.json" \
    --output "$build/android-build" \
    --android-platform android-34 \
    --jdk "$JAVA_HOME" \
    --gradle \
    --release \
    --sign "$keystore" "$keyAlias" \
    --storepass "$storePassword" \
    --keypass "$keyPassword"

ver=$(sed -n 's/^android:ANDROID_VERSION_NAME = //p' "$root/DroidStar.pro" | head -1 | tr -d '[:space:]')
[ -n "$ver" ] || ver=0.45.07a
mkdir -p "$root/dist"
apk_src=$build/android-build/build/outputs/apk/release/android-build-release-signed.apk
apk_dst=$root/dist/DroidStar-$ver-arm64.apk
cp -f "$apk_src" "$apk_dst"

python3 - "$apk_dst" <<'PY'
import sys, zlib, zipfile
apk = sys.argv[1]
needles = (
    b"Android build - V",
    b"Modification Copyright (C) 2026 M8NLR",
    b"Available update",
    b"73 de M8NLR",
    b"https://github.com/sappsys/DroidStar-Android",
)
with zipfile.ZipFile(apk) as z:
    blob = b"".join(z.read(n) for n in z.namelist() if n.endswith(".so"))
# QML lives zlib-compressed inside the Qt resource blob.
inflated = bytearray(blob)
pos = 0
while True:
    pos = blob.find(b"\x78\xda", pos)
    if pos < 0:
        break
    try:
        inflated.extend(zlib.decompress(blob[pos:pos + 512 * 1024]))
    except zlib.error:
        pass
    pos += 1
missing = [n.decode() for n in needles if n not in inflated]
if missing:
    print("APK missing About/branding markers:", ", ".join(missing), file=sys.stderr)
    sys.exit(1)
print("About/branding markers OK")
PY

echo "APK: $apk_dst (built lib $lib_hash)"
