# Windows changelog

Windows letter releases of DroidStar. Upstream notes stay with VU3LVO: [vu3lvo.com/droidstar/changelog](https://www.vu3lvo.com/droidstar/changelog). Android letters are in [CHANGELOG-Android.md](CHANGELOG-Android.md).

Android and Windows share the same product letter for this line (`0.45.07a`, then `b`, and so on). A letter is assigned only when that build is updated on the public repo. A new upstream version starts again at `a`. Shared features added on either platform are brought across to the other before the next letter ships, unless a note here marks them as platform-only (for example Android APK updates, or Windows USB serial dongles).

## Unreleased

Not on the public repo yet. These notes move under the next letter when that update is published. Local builds already report **0.45.07c**, matching Android.

## 0.45.07b

First public Windows build from this tree. Shared letter with Android. Package name `DroidStar-0.45.07b-windows-x64` (folder or zip).

- First Windows packaging in this tree: `windows/`, `scripts/build-windows.ps1`, self-contained x64 folder via `windeployqt`, optional Inno Setup. Same shared radio code and `ui2026` UI as Android, plus Qt SerialPort for USB AMBE / MMDVM dongles.
- About identifies this as a Windows build made by M8NLR. Splash, drawer, About wordmark, window icon, and EXE icon use the same M8NLR imagery as the Android line.
- Doug McLain remains original copyright. Rohith Namboothiri VU3LVO is listed as upstream copyright. Modification copyright is M8NLR, 2026.
- Buy Me a Coffee, PayPal, and the official-website button are hidden on Windows. The GitHub link opens [sappsys/DroidStar-Android](https://github.com/sappsys/DroidStar-Android).
- The splash line says 73 de M8NLR. The drawer and the home footer do not show a Made with VU3LVO line.
- The important notice says this Windows version is made by M8NLR and is available for public release.
- About can export a copy of the settings and, only when Restore is pressed, merge that file into the settings already in the app (native file dialogs). Keys this version already has, and the file does not, stay. The file includes the hotspot passwords. After restore the app restarts.
- Package name is `DroidStar-<version>-windows-x64` (folder or zip). Users do not need Qt installed.
- Not in Windows: Android in-app APK Update. iOS-only pieces stay on iOS (including Bluetooth LE PTT learn/scan). Settings no longer shows a dead Add Bluetooth button; keyboard-type PTT remains.
- Platform-only for now: Bluetooth LE hardware PTT (iOS CoreBluetooth). A Windows/Android BLE path would be new work, not a stub.
- Full headset support: TX Noise Reduction, TX HD Voice, and Mic Leveler follow the user settings on built-in, USB, wired headset, and Bluetooth mics (no more auto-bypass when an external mic is selected). Clarity-card taps use `preventStealing` so the main scroll view does not eat clicks.
- Settings toggle tiles (including TX HD Voice Clarity) no longer fight the Switch control: the row press handler stops short of the switch and ignores no-op toggles, so desktop clicks cannot double-fire and snap the switch back.
- TX HD Voice / NR / Mic Leveler no longer snap off when you key up: PTT starts capture, which used to re-detect the mic and push a full settings sync that overwrote those toggles. Mic tracking no longer emits that sync, and the UI keeps the user’s TX clarity choices.
- Neural AI TX clarity (GTCRN via sherpa-onnx) is enabled on Windows, matching iOS/Android: the package includes `sherpa-onnx-c-api.dll`, `onnxruntime.dll`, and the bundled `gtcrn_simple.onnx` model already in resources. TX HD Voice / Neural path uses the same OnlineSpeechDenoiser code as iOS.
- Startup crash with Neural AI fixed: the sherpa-onnx C API header matches the v2.0.1 DLL layout (`dpdfnet` attenuation + `voxlock` fields).
