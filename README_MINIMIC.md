# MiniMic Link

MiniMic Link adalah aplikasi remote microphone untuk cloud gaming. Sender mengirim mikrofon melalui koneksi P2P SonoBus/AOO, sedangkan receiver Windows meneruskan audio ke VB-CABLE agar dapat dipilih sebagai microphone oleh FiveM, Discord, browser voice chat, atau game lain.

## Status prototipe

- UI minimal Sender/Receiver tersedia.
- Generate Group ID dan password tersedia.
- Server `aoo.sonobus.net:10998` dapat dijangkau dan dipakai untuk prototipe.
- Audio memakai mesin P2P AOO/Opus dari SonoBus.
- Build Windows standalone tersedia.
- Build Android sender debug tersedia.
- Receiver hanya dapat Connect jika memilih `CABLE Input`.
- Sender selalu memblokir audio balik.
- Receiver selalu memblokir mikrofon lokal.
- Monitoring input ke speaker dipaksa nol.
- Privacy Mute tersedia.
- Meter level minimal tersedia pada Sender dan Receiver.
- Status koneksi menampilkan peer tujuan, misalnya `Terhubung ke sender-...` atau `Terhubung ke receiver-...`.
- Android memakai foreground microphone service agar mikrofon tetap aktif ketika aplikasi lain dibuka.
- Tes lokal Android sender → Windows receiver → VB-CABLE berhasil.
- Receiver menolak Connect jika default speaker Windows masih perangkat VB-CABLE, karena itu akan membuat audio desktop/game masuk ke jalur mic virtual.

Belum selesai pada tahap ini:

- Pengujian audio end-to-end dengan dua mesin di negara/jaringan berbeda.
- Enkripsi end-to-end. Protokol SonoBus yang menjadi basis prototipe belum mengenkripsi trafik audio.
- Installer Windows.
- Build iOS final karena membutuhkan macOS/Xcode/signing.

## Artefak siap pakai

- Windows x64: `dist\MiniMic-Link-Windows-x64-0.1.0.zip`
- Android debug APK dipisah per ABI agar ukuran download kecil:
  - `MiniMic-Link-Android-arm64-v8a-0.1.0-debug.apk` untuk mayoritas HP Android modern.
  - `MiniMic-Link-Android-armeabi-v7a-0.1.0-debug.apk` untuk HP Android 32-bit/lama.
  - `MiniMic-Link-Android-x86_64-0.1.0-debug.apk` untuk emulator Android Intel 64-bit.
  - `MiniMic-Link-Android-x86-0.1.0-debug.apk` untuk emulator Android Intel 32-bit.

Build Android ini debug-signed, cocok untuk install manual dan pengujian. Untuk distribusi publik tetap perlu release signing.
Jika ragu dan memakai HP Android biasa keluaran beberapa tahun terakhir, pilih `arm64-v8a`.

## Menjalankan aplikasi Windows

Executable hasil build berada di:

```text
build\SonoBus_artefacts\Release\Standalone\MiniMic Link.exe
```

### Receiver pada cloud PC

1. Install VB-Audio Virtual Cable.
2. Jalankan MiniMic Link dan pilih **Receiver**.
3. Pastikan output yang dipilih adalah **CABLE Input (VB-Audio Virtual Cable)**.
4. Pastikan default speaker Windows tetap speaker/headphone asli, misalnya **Steam Streaming Speakers**, bukan perangkat VB-CABLE.
5. Buat atau salin Group ID dan password.
6. Tekan **Connect**.
7. Di FiveM, Discord, atau game lain, pilih **CABLE Output (VB-Audio Virtual Cable)** sebagai microphone.

Audio receiver tidak diarahkan ke speaker/headphone. Jika `CABLE Input` tidak ditemukan, atau default speaker Windows masih perangkat VB-CABLE, aplikasi menolak koneksi receiver.

### Sender pada Windows

1. Jalankan MiniMic Link dan pilih **Sender**.
2. Pilih microphone sumber.
3. Masukkan Group ID dan password yang sama dengan receiver.
4. Tekan **Connect**.
5. Gunakan **Mute Mic** untuk menghentikan pengiriman tanpa keluar dari grup.
6. Meter **Level mic dikirim** menunjukkan apakah mikrofon benar-benar masuk.
7. Setelah terhubung, status menampilkan nama receiver agar pengguna tahu tidak salah peer.

### Sender pada Android

1. Install APK debug.
2. Jalankan MiniMic Link. Pada Android mode yang dipakai adalah **Sender**.
3. Izinkan permission mikrofon saat diminta.
4. Pilih microphone/input yang tersedia.
5. Masukkan Group ID dan password yang sama dengan receiver Windows.
6. Tekan **Connect**.
7. Pastikan status berubah menjadi **Terhubung ke receiver-...** dan meter level bergerak.

Android tidak menjadi receiver karena routing ke `CABLE Input/Output` hanya tersedia di Windows pada MVP ini.

Android sudah dites tetap mengirim suara ketika Chrome dibuka di depan selama lebih dari 20 detik. Foreground microphone service harus tetap aktif; jika sistem Android tertentu mematikan service karena battery saver/vendor policy, nonaktifkan battery optimization untuk MiniMic Link.

## Build Windows

Prasyarat:

- Windows 10/11 x64.
- CMake.
- Visual Studio 2022 Build Tools dengan workload C++ Desktop.

Jalankan:

```powershell
.\scripts\build-minimic-windows.ps1
```

ASIO dinonaktifkan karena aplikasi memakai WASAPI dan VB-CABLE. Build default hanya menghasilkan aplikasi standalone, bukan VST/AU/LV2.

## Build Android

Detail build mobile ada di `MOBILE_BUILD.md`.

Ringkasnya:

```powershell
.\scripts\build-minimic-android.ps1
```

APK debug akan dibuat di `mobile\Builds\Android\app\build\outputs\apk` dan disalin manual/otomatis ke `dist` sesuai kebutuhan rilis internal.
Script build menyalin APK split per ABI ke `dist`.

## Model routing

```text
Sender mic
  → Opus/AOO P2P
  → MiniMic Link Receiver
  → CABLE Input
  → CABLE Output
  → aplikasi yang membutuhkan microphone
```

`CABLE Output` boleh dipilih oleh aplikasi apa pun yang membutuhkan mic. “Tidak bocor” berarti MiniMic Link tidak memutar audio sender ke speaker/headphone dan tidak merekam audio tersebut.

## Hasil tes lokal terakhir

- Android sender dan Windows receiver berhasil terhubung melalui `aoo.sonobus.net:10998`.
- Windows receiver memakai `CABLE Input (VB-Audio Virtual Cable)`.
- Status peer tampil: receiver melihat sender, sender melihat receiver.
- Audio foreground masuk ke `CABLE Output` dengan RMS sekitar `-42.4 dBFS`.
- Ketika **Mute Mic** aktif di sender, `CABLE Output` turun ke noise floor sekitar `-95.6 dBFS`.
- Ketika Chrome dibuka di Android dan MiniMic berada di background, audio tetap masuk ke `CABLE Output` dengan RMS sekitar `-37.4 dBFS`.

## Lisensi

Proyek ini merupakan modifikasi dari SonoBus dan mengikuti GPLv3 serta ketentuan source upstream yang disertakan di repository. Source asli SonoBus tersedia di <https://github.com/sonosaurus/sonobus>.
