# Mobile build — MiniMic Link

Android dan iOS hanya berperan sebagai **Sender**. Receiver tetap Windows karena membutuhkan `CABLE Input/Output`.

## Status saat ini

- Android debug APK sudah berhasil dibuild dan diverifikasi metadata/signature.
- APK final untuk testing: `dist\MiniMic-Link-Android-0.1.0-debug.apk`
- Package Android: `app.minimic.link`
- Label aplikasi: `MiniMic Link`
- Minimum Android: API 24 / Android 7.0.
- Target Android: API 33.
- ABI yang dibuild: `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`.
- Sudah diverifikasi pada perangkat Android fisik melalui wireless ADB.
- Foreground microphone service aktif dengan `foregroundServiceType="microphone"`.
- Tes background: Android sender tetap mengirim audio ketika Chrome berada di foreground selama lebih dari 20 detik.

## Android

Prasyarat:

- JDK 17.
- Android SDK Platform 33.
- Android NDK `25.2.9519653`.
- CMake Android `3.22.1`.
- Environment variable `ANDROID_SDK_ROOT` atau `ANDROID_HOME`.

Build debug:

```powershell
.\scripts\build-minimic-android.ps1
```

Debug build memakai debug signing bawaan Android. Release signing harus dibuat khusus untuk MiniMic Link sebelum distribusi.

Pada mesin ini Android SDK dipasang di:

```text
C:\Users\Admin\AppData\Local\Android\Sdk
```

Paket yang dipakai:

- `platforms;android-33`
- `build-tools;33.0.2`
- `platform-tools`
- `ndk;25.2.9519653`
- `cmake;3.22.1`

Install manual ke perangkat Android, jika USB debugging aktif:

```powershell
adb install -r .\dist\MiniMic-Link-Android-0.1.0-debug.apk
```

Jika Android meminta permission mikrofon, izinkan. Untuk sesi gaming panjang, disarankan menonaktifkan battery optimization khusus untuk MiniMic Link agar foreground service tidak dihentikan oleh vendor ROM.

## iOS

iOS memerlukan macOS, Xcode, dan signing team milik pengembang.

1. Buka `mobile/SonoBusMobile.jucer` memakai Projucer dari JUCE yang dibundel.
2. Regenerate proyek iOS ke `mobile/Builds/iOS`.
3. Buka proyek Xcode yang dihasilkan.
4. Ganti Bundle Identifier dan Development Team ke milik pengembang.
5. Build target Standalone pada iPhone nyata.

File `.jucer` sudah memasukkan `MiniMicEditor.cpp/.h`, mengaktifkan `MINIMIC_MINIMAL_UI=1`, dan hanya memilih format Standalone. Proyek iOS generated lama harus diregenerasi sebelum build.
