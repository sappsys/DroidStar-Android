# Android changelog

Android letter releases of DroidStar. Upstream notes stay with VU3LVO: [vu3lvo.com/droidstar/changelog](https://www.vu3lvo.com/droidstar/changelog). Windows letters are in [CHANGELOG-Windows.md](CHANGELOG-Windows.md).

Android and Windows share the same product letter for this line (`0.45.07a`, then `b`, and so on). A letter is assigned only when that build is updated on the public repo. A new upstream version starts again at `a`. Further public builds of the same upstream version use the next letter. The package code is the upstream number times 100, plus the letter index (`a` is 1). Shared features added on either platform are brought across to the other before the next letter ships, unless a note here marks them as platform-only.

## Unreleased

Not on the public repo yet. These notes move under the next letter when that update is published. Local builds already report **0.45.07c**, matching Windows.

## 0.45.07b

Package code 450702. Tracks upstream 0.45.07. Shared letter with Windows.

- Windows is another product target in this same tree (not a fork). Packaging and Windows-only notes live in [CHANGELOG-Windows.md](CHANGELOG-Windows.md).
- Settings hides Add Bluetooth button when BLE PTT is not supported (Android/Windows). Keyboard-type PTT stays. BLE learn/scan remains iOS-only until a shared Qt Bluetooth path exists.
- Full headset support: TX Noise Reduction, TX HD Voice, and Mic Leveler stay under user control on built-in, USB, wired headset, and Bluetooth mics (shared with Windows; no auto-bypass when an external mic is selected).
- TX clarity toggles no longer snap off on PTT (shared with Windows): capture start must not overwrite the user’s NR / HD Voice / Mic Leveler choices.
- Neural AI TX clarity (GTCRN via sherpa-onnx) is enabled on Android arm64, matching iOS/Windows: `libonnxruntime.so` and `libsherpa-onnx-c-api.so` ship in the APK; the bundled `gtcrn_simple.onnx` model is already in resources. Header layout matches sherpa-onnx v2.0.1 so model warm-up at launch does not fault.

## 0.45.07a

Package code 450701. Tracks upstream 0.45.07. First public Android build from that tree.

- Arm64 package for Android 12 and newer.
- BrandMeister live scan can connect. The build includes the OpenSSL 3 libraries Qt’s Android TLS plugin loads.
- iOS-only pieces stay on iOS: Live Activity, Bluetooth PTT, and Apple Speech. Keyboard PTT and QSO replay (WAV) are in the Android build. (Neural AI / sherpa GTCRN arrived in 0.45.07b.)
- Filtering the Scan list, then opening Settings, no longer freezes the display. Scan stops updating before the menu closes, Settings opens on the next frame, and the live card grid no longer feeds a layout loop.
- About identifies this as an Android build made by M8NLR.
- Doug McLain remains original copyright. Rohith Namboothiri VU3LVO is listed as upstream copyright. Modification copyright is M8NLR, 2026.
- Buy Me a Coffee, PayPal, and the official-website button are hidden on Android. The GitHub link opens [sappsys/DroidStar-Android](https://github.com/sappsys/DroidStar-Android).
- The splash line says 73 de M8NLR. The drawer and the home footer do not show a Made with VU3LVO line.
- New launcher icon for this Android line.
- The About page, splash, and drawer use a DroidStar wordmark that reads “Android by M8NLR”.
- The important notice says this Android version is made by M8NLR and is available for public release.
- About has an Update button. It shows the installed version and the newer public release when one exists. Available update says Checking, then the version, None, or Check Failed.
- About can export a copy of the settings and, only when Restore is pressed, merge that file into the settings already in the app. Keys this version already has, and the file does not, stay. The file includes the hotspot passwords.
- Local builds use the same release keystore as the APK published on GitHub. The package name is `DroidStar-<version>-arm64.apk`.
