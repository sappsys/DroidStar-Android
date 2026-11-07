# DroidStar Android

Android builds of [DroidStar](https://github.com/nostar/DroidStar), tracking upstream **0.45.07**. This Android release is **0.45.07a**.

DroidStar is a free, GPL digital ham radio app: DMR on BrandMeister, YSF, M17, P25, NXDN, D-STAR, and AllStar. You need an amateur licence. The phone talks to the network the way a hotspot would.

This repository puts Android back on that line. Upstream stopped shipping Android and continues the iPhone and iPad app. We take those updates and bundle them into our Android build. The phone app is still named **DroidStar**, package `org.dudetronics.droidstar`, so a new build replaces the previous one. This repository is the Android line, not the official iOS app.

## Credit

- **Doug McLain** wrote the original DroidStar: [nostar/DroidStar](https://github.com/nostar/DroidStar).
- **Rohith Namboothiri, VU3LVO** maintains the current app and the iOS work these builds are made from: [rohithzmoi/Droidstar-DMR](https://github.com/rohithzmoi/Droidstar-DMR), [vu3lvo.com/droidstar](https://vu3lvo.com/droidstar), changelog at [vu3lvo.com/droidstar/changelog](https://www.vu3lvo.com/droidstar/changelog).
- **VU2TBR (T. Bala)** is credited by VU3LVO for the feedback that shaped much of what is in 0.45.07.

The iOS app, TestFlight invites, and that changelog stay with VU3LVO. Questions about the official iPhone and iPad app go to [hi@vu3lvo.com](mailto:hi@vu3lvo.com).

## What this line does

- Builds Android from the current upstream sources. Upstream 0.45.07 is the base of Android 0.45.07a.
- Merges later upstream updates and ships them as the next Android build.
- Leaves the iOS project intact. Android work is in the `android:` project settings, the `android/` package, and `Q_OS_ANDROID` sections.

Further Android builds of the same upstream version use the next letter: `0.45.07b`, then `c`, through `z`. The package code is `450700` plus the letter index (`a` is `450701`) so each letter upgrades in place. The next upstream version, `0.45.08`, can start at `450800`.

## This Android build

`scripts/build-android.sh` builds the arm64 package and signs it with the release keystore in `android/release/`. That is the APK you install locally and the APK published on GitHub. It is written to `dist/DroidStar-<version>-arm64.apk`. The script expects Qt 6.6.3 for Android (`android_arm64_v8a`), NDK 25.1, and JDK 17. `scripts/package-android-release.sh` runs the same build.

The keystore directory is not committed. Keep a copy of the keystore and `signing.properties`; a later public build has to use the same key.

The BrandMeister live scanner is HTTPS. Qt for Android does not ship the OpenSSL libraries its TLS plugin loads, so this tree vendors OpenSSL 3.1.8 under `third_party/android_openssl/` (Apache-2.0; see that directory's `LICENSE`).

Shared radio modes, keyboard PTT, and QSO replay (saved as WAV) are in the Android build. Live Activity, Bluetooth PTT, Apple Speech, and the sherpa noise model stay on iOS, where upstream ships them.

What 0.45.07 added upstream — QSO Replay, hardware PTT, Simple mode, the BrandMeister and TGIF password check, and the rest of that release — is described in VU3LVO's [changelog](https://www.vu3lvo.com/droidstar/changelog). Each Android letter is recorded in [CHANGELOG-Android.md](CHANGELOG-Android.md).

## Taking upstream updates

`upstream` is [rohithzmoi/Droidstar-DMR](https://github.com/rohithzmoi/Droidstar-DMR), branch `Main`.

```
git fetch upstream
git merge upstream/Main
```

After a merge that brings a new upstream version, set that Android release back to `a` when it is updated on the public repo. Update the package code in `DroidStar.pro` and [CHANGELOG-Android.md](CHANGELOG-Android.md), then rebuild. The APK name comes from the version in `DroidStar.pro`.

## License

GNU General Public License v3.0. Copyright remains with the original authors. See [LICENSE](LICENSE) and the headers in the source files.

Logs, settings, favourite talkgroups, net reminders, transcripts, message history, and QSO replays stay on the device. The networks you connect to still see your callsign and DMR ID, the same way they would from a radio. VU3LVO's privacy note for the official app is at [vu3lvo.com/droidstar/privacy](https://vu3lvo.com/droidstar/privacy).
