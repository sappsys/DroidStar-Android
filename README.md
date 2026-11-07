# DroidStar Android

Android builds of [DroidStar](https://github.com/nostar/DroidStar), tracking upstream **0.45.07**. The current public release is **0.45.07c** (Android APK and Windows x64 zip).

DroidStar is a free, GPL digital ham radio app: DMR on BrandMeister, YSF, M17, P25, NXDN, D-STAR, and AllStar. You need an amateur licence. The phone talks to the network the way a hotspot would.

This repository puts Android back on that line. Upstream stopped shipping Android and continues the iPhone and iPad app. We take those updates and bundle them into our Android build. The phone app is still named **DroidStar**, package `org.dudetronics.droidstar`, so a new build replaces the previous one. This repository is the Android line, not the official iOS app.

## Credit

- **Doug McLain** wrote the original DroidStar: [nostar/DroidStar](https://github.com/nostar/DroidStar).
- **Rohith Namboothiri, VU3LVO** maintains the current app and the iOS work these builds are made from: [rohithzmoi/Droidstar-DMR](https://github.com/rohithzmoi/Droidstar-DMR), [vu3lvo.com/droidstar](https://vu3lvo.com/droidstar), changelog at [vu3lvo.com/droidstar/changelog](https://www.vu3lvo.com/droidstar/changelog).
- **VU2TBR (T. Bala)** is credited by VU3LVO for the feedback that shaped much of what is in 0.45.07.

The iOS app, TestFlight invites, and that changelog stay with VU3LVO. Questions about the official iPhone and iPad app go to [hi@vu3lvo.com](mailto:hi@vu3lvo.com).

## What this line does

- Builds Android from the current upstream sources. Upstream 0.45.07 is the base of this letter line (public **0.45.07c**).
- Merges later upstream updates and ships them as the next Android build.
- Leaves the iOS project intact. Android work is in the `android/` package and `Q_OS_ANDROID` sections. Windows packaging lives in `windows/` with `Q_OS_WIN` / desktop gates — same product tree, not a fork.

Android and Windows share one product letter for this line: `0.45.07a`, then `b`, through `z`. Further builds of the same upstream version use the next letter on both platforms together. The Android package code is `450700` plus the letter index (`a` is `450701`) so each letter upgrades in place. The next upstream version, `0.45.08`, can start at `450800`. Shared features added on Android or Windows are brought across to the other before the next letter ships, unless a changelog note marks them as platform-only.

## This Android build

`scripts/build-android.sh` builds the arm64 package and signs it with the release keystore in `android/release/`. That is the APK you install locally and the APK published on GitHub. It is written to `dist/DroidStar-<version>-arm64.apk`. The script expects Qt 6.6.3 for Android (`android_arm64_v8a`), NDK 25.1, and JDK 17. `scripts/package-android-release.sh` runs the same build.

The keystore directory is not committed. To build a signed APK you need your own keystore and a matching `signing.properties` in `android/release/`. The published GitHub APKs use the project release key, which is not in this repository; keep a private copy of that keystore if you will ship further upgrades of the same package.

The BrandMeister live scanner is HTTPS. Qt for Android does not ship the OpenSSL libraries its TLS plugin loads, so this tree vendors OpenSSL 3.1.8 under `third_party/android_openssl/` (Apache-2.0; see that directory's `LICENSE`).

Shared radio modes, keyboard PTT, QSO replay (saved as WAV), and Neural AI TX clarity (GTCRN via sherpa-onnx) are in the Android build. Live Activity, Bluetooth PTT, and Apple Speech stay on iOS.

### Windows (desktop)

Windows uses the same shared radio code and `ui2026` UI, plus Qt SerialPort for USB AMBE / MMDVM dongles (the nostar desktop path). Packaging mirrors Android: see [windows/README.md](windows/README.md). On a Windows machine with Qt 6.6 x64:

```
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
```

That writes a self-contained folder under `dist/DroidStar-<version>-windows-x64/` (`windeployqt`; no Qt install needed on the user’s PC), then builds the `.zip` from that folder so both stay in line. Optional Inno Setup script: `windows/installer/DroidStar.iss`.

What 0.45.07 added upstream — QSO Replay, hardware PTT, Simple mode, the BrandMeister and TGIF password check, and the rest of that release — is described in VU3LVO's [changelog](https://www.vu3lvo.com/droidstar/changelog). Each Android letter is recorded in [CHANGELOG-Android.md](CHANGELOG-Android.md); each Windows letter in [CHANGELOG-Windows.md](CHANGELOG-Windows.md).

## Taking upstream updates

`upstream` is [rohithzmoi/Droidstar-DMR](https://github.com/rohithzmoi/Droidstar-DMR), branch `Main`.

```
git fetch upstream
git merge upstream/Main
```

After a merge that brings a new upstream version, set that product letter back to `a` when it is updated on the public repo (Android and Windows together). Update the version in `DroidStar.pro`, [CHANGELOG-Android.md](CHANGELOG-Android.md), and [CHANGELOG-Windows.md](CHANGELOG-Windows.md), then rebuild both. The APK and Windows folder names come from that shared version.

## License

GNU General Public License v3.0. Copyright remains with the original authors. See [LICENSE](LICENSE) and the headers in the source files.

Logs, settings, favourite talkgroups, net reminders, transcripts, message history, and QSO replays stay on the device. The networks you connect to still see your callsign and DMR ID, the same way they would from a radio. VU3LVO's privacy note for the official app is at [vu3lvo.com/droidstar/privacy](https://vu3lvo.com/droidstar/privacy).
