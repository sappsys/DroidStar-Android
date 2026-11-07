#!/bin/sh
# Build the arm64 debug APK. Requires the Qt 6.6.3 Android kit and NDK 25.1.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
: "${QT_ANDROID:=/opt/Qt/6.6.3/android_arm64_v8a}"
: "${ANDROID_SDK_ROOT:=/opt/android-sdk}"
: "${ANDROID_NDK_ROOT:=$ANDROID_SDK_ROOT/ndk/25.1.8937393}"
: "${JAVA_HOME:=/usr/lib/jvm/java-17-openjdk-amd64}"
export ANDROID_SDK_ROOT ANDROID_NDK_ROOT JAVA_HOME
export PATH="$JAVA_HOME/bin:$ANDROID_SDK_ROOT/platform-tools:$PATH"

build=$root/build/android
mkdir -p "$build"
cd "$build"
"$QT_ANDROID/bin/qmake" "$root/DroidStar.pro" -spec android-clang CONFIG+=release
make -j"$(nproc)"
make apk
mkdir -p "$root/dist"
cp -f "$build/android-build/build/outputs/apk/debug/android-build-debug.apk" \
    "$root/dist/DroidStar-0.45.07a-arm64-debug.apk"
echo "APK: $root/dist/DroidStar-0.45.07a-arm64-debug.apk"
