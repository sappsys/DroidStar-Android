# Windows packaging

Windows is a first-class target of this product tree, same idea as `android/`:
shared radio + `ui2026` QML, with packaging and Windows-only glue living here.

## What you get

`scripts/build-windows.ps1` builds a **self-contained folder**:

```
dist/DroidStar-<version>-windows-x64/
  DroidStar.exe
  Qt*.dll, platforms/, qml/, …   (via windeployqt)
  sherpa-onnx-c-api.dll, onnxruntime*.dll   (Neural AI TX clarity)
```

Zip that folder, or compile `windows/installer/DroidStar.iss` with Inno Setup for a setup.exe. Users do not need Qt installed.

A single statically linked EXE is out of scope for v1.

## Requirements (on a Windows machine)

- Qt 6.6.x for **MSVC 2019/2022 64-bit** or MinGW 64-bit (same major as the Android line where practical)
- Matching Qt modules: Quick, Quick Controls 2, Network, Multimedia, Serial Port, Concurrent
- Visual Studio Build Tools (for MSVC) **or** MinGW from the Qt installer
- JDK is not required

USB AMBE / MMDVM dongles use Qt SerialPort (same code path as nostar desktop). Install the dongle’s COM-port driver first; the device then appears under Settings → Vocoder / Modem.

## Build

From a “x64 Native Tools” or Qt command prompt, in the repo root:

```powershell
.\scripts\build-windows.ps1
```

Optional:

```powershell
$env:QT_DIR = "C:\Qt\6.6.3\msvc2019_64"
.\scripts\build-windows.ps1
```

Output lands in `dist\DroidStar-<version>-windows-x64\`.

## Installer (optional)

1. Install [Inno Setup](https://jrsoftware.org/isinfo.php)
2. Build the app folder first
3. Open `windows/installer/DroidStar.iss` and compile

## Version and feature parity

Windows uses the same `APP_VERSION` letter as Android (`DroidStar.pro`). Bump that letter once for the product line when you publish; record both [CHANGELOG-Android.md](../CHANGELOG-Android.md) and [CHANGELOG-Windows.md](../CHANGELOG-Windows.md). Shared features added on either platform are brought across to the other before that letter ships, unless a changelog note marks them as platform-only.

## Not in the Windows build

- Android in-app APK Update
- iOS Live Activity, BLE PTT learning, Apple Speech

Keyboard PTT, QSO Replay (WAV), BrandMeister scan, DMR SMS, settings export/restore, Neural AI TX clarity (GTCRN), and serial dongles are in scope. The build script copies sherpa-onnx / onnxruntime DLLs from `third_party/sherpa-onnx/lib/win-x64/` into the package.
