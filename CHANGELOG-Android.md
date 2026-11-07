# Android changelog

Android letter releases of DroidStar. Upstream notes stay with VU3LVO: [vu3lvo.com/droidstar/changelog](https://www.vu3lvo.com/droidstar/changelog).

A letter is assigned only when that build is updated on the public repo. A new upstream version starts again at `a`. Further public builds of the same upstream version use the next letter. The package code is the upstream number times 100, plus the letter index (`a` is 1).

## Unreleased

Not on the public repo yet. These notes move under the next letter when that update is published.

## 0.45.07a

Package code 450701. Tracks upstream 0.45.07. First public Android build from that tree.

- Arm64 package for Android 12 and newer.
- BrandMeister live scan can connect. The build includes the OpenSSL 3 libraries Qt’s Android TLS plugin loads.
- iOS-only pieces stay on iOS: Live Activity, Bluetooth PTT, Apple Speech, and the sherpa noise model. Keyboard PTT and QSO replay (WAV) are in the Android build.
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
