# Blueprint — Mini Mic Sender & Receiver untuk Cloud Gaming

## 1. Ringkasan produk

Aplikasi ini mengirimkan suara mikrofon dari perangkat pemain melalui internet ke PC Windows yang menjalankan cloud gaming. Di PC penerima, suara diteruskan menjadi **virtual microphone** agar dapat dipilih sebagai input oleh FiveM, Discord, atau game/aplikasi voice chat lain.

Produk dibuat sesederhana mungkin: pengguna cukup memakai **Group ID** dan **password** yang sama pada sender dan receiver, lalu menekan **Connect**.

### Platform

- **Sender:** Android, iOS, dan Windows.
- **Receiver:** Windows.
- **Target utama:** satu sender terhubung ke satu receiver.
- **Pengembangan berikutnya:** satu sender ke beberapa receiver atau beberapa sender dalam satu grup.

### Contoh penggunaan

```text
Android/iPhone/PC pemain
          │
          │  audio Opus melalui UDP/P2P
          ▼
Receiver pada Windows cloud PC
          │
          ▼
Virtual Microphone Windows
          │
          ▼
FiveM / Discord / game lain
```

## 2. Tujuan

1. Koneksi cukup menggunakan Group ID dan password.
2. Audio tetap jelas pada koneksi lintas negara yang wajar.
3. Latensi aplikasi dibuat serendah mungkin; keterlambatan jaringan fisik tetap menjadi batas utama.
4. Receiver dapat dipilih sebagai mikrofon oleh aplikasi Windows apa pun yang membutuhkan mic, misalnya FiveM, Discord, atau game lain.
5. Aplikasi dapat reconnect otomatis ketika jaringan berpindah atau terputus singkat.
6. Antarmuka minimal dan dapat digunakan tanpa pengetahuan jaringan.
7. Audio sender tidak diputar ke speaker receiver dan tidak direkam/disimpan oleh aplikasi.

## 3. Batas MVP

### Termasuk

- Audio satu arah: sender ke receiver.
- Satu sender dan satu receiver per sesi.
- Signaling untuk mempertemukan peer berdasarkan Group ID.
- Autentikasi menggunakan password grup.
- Koneksi media P2P melalui UDP jika memungkinkan.
- Relay sebagai fallback ketika NAT/firewall memblokir P2P, setelah infrastrukturnya tersedia.
- Codec Opus mono.
- Adaptive jitter buffer, packet-loss concealment, dan reconnect otomatis.
- Pemilihan mikrofon pada sender.
- Pemilihan output/virtual audio device pada receiver.
- Routing audio eksklusif ke virtual audio device yang dipilih; monitoring ke speaker nonaktif secara default.
- Mute, indikator level suara, status koneksi, dan perkiraan latency.
- Windows receiver bekerja dengan virtual audio cable yang sudah terpasang.

### Belum termasuk

- Voice room banyak pengguna.
- Audio dua arah.
- Rekaman suara.
- Efek suara, voice changer, mixing, dan equalizer kompleks.
- Akun pengguna, daftar teman, dan histori grup.
- Virtual audio driver buatan sendiri.
- Dashboard administrasi lengkap.

## 4. Pengalaman pengguna

### Sender

Komponen layar utama:

- Mode: **Sender**.
- Input Group ID.
- Input password dengan tombol tampil/sembunyikan.
- Tombol **Generate ID** dan **Generate Password**.
- Tombol **Connect / Disconnect**.
- Pilihan mikrofon.
- Tombol **Mute**.
- Indikator permanen ketika mikrofon sedang dikirim.
- Meter level mikrofon.
- Status: Disconnected, Connecting, Connected, Reconnecting, atau Error.
- Saat connected, status wajib menampilkan nama peer tujuan, misalnya `Terhubung ke receiver-...`.
- Informasi ringkas: ping/latency dan kualitas jaringan.

### Receiver Windows

Komponen layar utama:

- Mode: **Receiver**.
- Input Group ID dan password.
- Tombol **Connect / Disconnect**.
- Pilihan output audio, dengan virtual cable sebagai pilihan utama.
- Status **Private routing** yang memastikan speaker/monitoring lokal nonaktif.
- Meter level audio yang diterima.
- Status koneksi, latency, packet loss, dan nama sender yang terhubung.
- Tombol tes singkat untuk memastikan aplikasi target menerima audio.

### Alur penggunaan

1. Receiver membuat Group ID dan password, atau memakai data yang sudah disepakati.
2. Group ID dan password dibagikan ke sender melalui saluran pribadi.
3. Receiver memilih virtual audio device dan menekan Connect.
4. Sender memasukkan data yang sama lalu menekan Connect.
5. Signaling server mempertemukan kedua peer.
6. Aplikasi mencoba jalur P2P; jika gagal, aplikasi mencoba relay yang tersedia.
7. Receiver mengirim audio masuk ke virtual microphone.
8. Pengguna memilih `CABLE Output` sebagai microphone di FiveM, Discord, atau aplikasi lain yang membutuhkan mic.

Receiver **tidak** mengirim salinan audio ke speaker/headphone Windows. Monitoring hanya boleh tersedia sebagai mode tes sementara, harus diaktifkan secara sadar, menampilkan peringatan yang jelas, dan otomatis mati setelah tes selesai.

## 5. Arsitektur sistem

```text
┌────────────────────┐       ┌──────────────────────┐
│ Sender             │       │ Signaling Service    │
│ Android/iOS/Windows│◄─────►│ Pairing + NAT info   │
│                    │       └──────────────────────┘
│ Mic capture        │                  ▲
│ Noise processing   │                  │
│ Opus encoder       │                  ▼
│ Encrypt + UDP      │══════════════════════════════╗
└────────────────────┘       jalur P2P utama         ║
                                      ┌──────────────▼─────┐
                                      │ Windows Receiver   │
                                      │ UDP + decrypt      │
                                      │ Jitter buffer      │
                                      │ Opus decoder       │
                                      │ Audio output       │
                                      └──────────────┬─────┘
                                                     │
                                      ┌──────────────▼─────┐
                                      │ Virtual Mic Device │
                                      └──────────────┬─────┘
                                                     │
                                      ┌──────────────▼─────┐
                                      │ FiveM / Discord    │
                                      └────────────────────┘
```

### Komponen

1. **Client UI** — layar minimal untuk ID, password, perangkat audio, dan status.
2. **Shared audio/network core** — capture, Opus, packet sequencing, enkripsi, jitter buffer, dan reconnect.
3. **Signaling service** — pairing, pertukaran informasi koneksi, dan koordinasi NAT traversal.
4. **STUN/TURN atau relay setara** — membantu koneksi ketika direct P2P tidak dapat dibuat.
5. **Virtual audio device** — menjembatani audio receiver ke input mikrofon Windows.

## 6. Pilihan teknologi yang disarankan

### Jalur implementasi

Jalur pertama yang dicoba adalah memakai **AOO/shared native audio core** dengan server SonoBus/AOO yang tersedia, lalu membuat UI baru yang minimal. SonoBus dapat dijadikan referensi atau basis setelah audit lisensi, dependency, dukungan platform, dan kemampuan membangun ulang aplikasinya berhasil.

Alasan tidak langsung mengunci proyek ke fork penuh SonoBus:

- Produk hanya membutuhkan sebagian kecil fitur SonoBus.
- Menghapus UI dan fitur besar dari fork dapat menambah beban pemeliharaan.
- Endpoint resmi `aoo.sonobus.net:10998` menjadi target koneksi prototipe pertama. Endpoint tersebut tetap harus diuji izin penggunaan, kestabilan, kapasitas, dan kompatibilitas custom client sebelum dianggap layak.

### Susunan teknis

- **Core:** C/C++ agar audio real-time dapat dibagi ke semua platform.
- **Audio networking:** AOO jika hasil feasibility test memenuhi kebutuhan.
- **Codec:** Opus mono, 48 kHz.
- **Windows audio:** WASAPI.
- **Android audio:** Oboe/AAudio dengan fallback yang sesuai perangkat.
- **iOS audio:** AVAudioSession/Audio Unit.
- **UI:** UI lintas platform tipis atau UI native yang memanggil shared core. Keputusan final dibuat setelah proof of concept audio berjalan.
- **Virtual mic MVP:** virtual audio cable pihak ketiga.
- **Virtual mic final:** driver milik proyek hanya jika distribusi dan pengalaman instalasi memang memerlukannya.

## 7. Pipeline audio

### Sender

```text
Microphone
  → capture 48 kHz mono
  → high-pass/noise suppression (opsional dan dapat diatur)
  → automatic gain control ringan
  → Opus encode
  → sequence number + timestamp
  → autentikasi/enkripsi paket
  → UDP transport
```

### Receiver

```text
UDP transport
  → validasi + dekripsi
  → reorder/drop paket terlambat
  → adaptive jitter buffer
  → Opus decode + packet-loss concealment
  → resample bila format device berbeda
  → virtual audio output
  → aplikasi target membaca virtual microphone
```

### Profil audio awal

Nilai berikut merupakan titik awal dan harus disetel melalui pengujian nyata:

| Parameter | Nilai awal |
|---|---:|
| Sample rate | 48 kHz |
| Channel | Mono |
| Opus frame | 10 ms, fallback 20 ms |
| Bitrate | 24–48 kbps |
| Jitter buffer | Adaptif, mulai 20–40 ms |
| Opus in-band FEC | Aktif saat packet loss meningkat |
| DTX | Opsional; nonaktif dahulu untuk respons konsisten |

Jitter buffer tidak boleh dipaksa terlalu kecil. Buffer yang terlalu kecil menghasilkan audio putus-putus atau “keresek”; buffer terlalu besar menambah latency. Aplikasi harus menyesuaikannya berdasarkan kondisi jaringan.

## 8. Target latency dan kualitas

Target aplikasi tidak termasuk waktu propagasi internet yang tidak dapat dikendalikan.

| Bagian | Target awal |
|---|---:|
| Capture + buffering sender | 10–20 ms |
| Encode | ≤5 ms pada perangkat target |
| Jitter buffer receiver | 20–60 ms adaptif |
| Decode + output buffering | 10–20 ms |
| Overhead aplikasi di luar jaringan | sekitar 40–100 ms |

Latency total lintas negara akan ditambah waktu perjalanan jaringan. Jalur direct P2P dan lokasi cloud PC sangat menentukan hasil.

Kriteria kualitas MVP:

- Tidak ada crackle akibat buffer race, underrun, atau format audio yang salah.
- Audio tetap dapat dimengerti pada packet loss ringan.
- Tidak terjadi memory allocation atau operasi blocking di real-time audio callback.
- Reconnect tidak memerlukan restart aplikasi.
- Pergantian Wi-Fi/data seluler menghasilkan pemulihan sesi yang terkendali.

## 9. Signaling dan koneksi

### Fungsi signaling

Signaling hanya membantu peer saling menemukan dan bertukar informasi koneksi. Audio sebaiknya tidak melewati signaling server.

Data minimum sesi:

- Protocol version.
- Group ID atau room identifier yang sudah di-hash.
- Role: sender atau receiver.
- Session nonce.
- Informasi kandidat koneksi/NAT.
- Dukungan codec dan parameter audio.
- Bukti autentikasi tanpa mengirim password mentah.

### Urutan koneksi

```text
DISCONNECTED
    ↓
RESOLVING_SIGNAL_SERVER
    ↓
AUTHENTICATING_GROUP
    ↓
WAITING_FOR_PEER
    ↓
NEGOTIATING_P2P
    ├── berhasil → CONNECTED_DIRECT
    └── gagal    → CONNECTING_RELAY → CONNECTED_RELAY
    ↓
RECONNECTING saat jaringan terputus
```

### Aturan ID dan password

- Group ID harus mudah dibaca/diketik dan memiliki entropy yang cukup.
- Password hasil generate harus acak, bukan turunan dari Group ID.
- Password tidak boleh dikirim atau disimpan sebagai plaintext.
- UI harus menyediakan copy/share yang jelas, tanpa mencatat password dalam log.
- Percobaan koneksi harus diberi rate limit untuk menghambat brute force.

## 10. Keamanan

Persyaratan minimum untuk versi final:

- Enkripsi end-to-end untuk paket audio.
- Autentikasi peer berdasarkan password grup.
- Kunci sesi unik untuk setiap koneksi.
- Nonce paket tidak boleh digunakan ulang.
- Sequence number digunakan untuk menolak replay paket.
- Log diagnostik tidak boleh berisi password, key, atau audio.
- Signaling menggunakan transport terenkripsi.
- Server tidak boleh dapat masuk ke grup hanya dengan mengetahui Group ID.
- Aplikasi tidak merekam, menyimpan, atau membuat cache berisi audio.
- Sender hanya membuka mikrofon saat pengguna menghubungkan sesi; indikator mic harus selalu terlihat selama capture aktif.
- Mute menghentikan pengiriman audio dan mengirim silence/control state yang aman.
- Receiver membersihkan buffer audio sementara saat disconnect.
- Jalur audio receiver hanya boleh menuju virtual device yang dipilih, bukan default speaker.

Detail kriptografi harus memakai primitive/library yang telah diaudit, bukan algoritma buatan sendiri. Mekanisme autentikasi dan derivasi key final dipilih berdasarkan kemampuan AOO serta library yang tersedia setelah feasibility test.

Catatan prototype: build awal berbasis SonoBus/AOO memakai group password untuk masuk grup, tetapi belum menyediakan enkripsi end-to-end untuk trafik audio. Karena itu build ini cocok untuk feasibility dan testing internal, bukan untuk penggunaan sensitif sampai lapisan keamanan final selesai.

## 11. Integrasi virtual microphone Windows

### MVP

Gunakan virtual audio cable yang sudah terpasang:

1. Receiver memilih **CABLE Input** sebagai output audio.
2. Receiver menulis PCM hasil decode ke **CABLE Input**.
3. FiveM, Discord, atau game lain memilih **CABLE Output** sebagai input microphone.
4. Receiver menampilkan peringatan jika device hilang atau formatnya tidak kompatibel.
5. Receiver membuka hanya endpoint virtual cable yang dipilih dan tidak membuka endpoint speaker untuk monitoring.

Nama `CABLE Input` dan `CABLE Output` mengikuti virtual audio cable yang dipakai untuk MVP. Aplikasi tetap menampilkan daftar perangkat agar pengguna dapat memilih instalasi cable yang benar.

### Aturan routing virtual microphone

`CABLE Output` memang ditujukan sebagai microphone sistem untuk semua aplikasi yang membutuhkannya. Akses tidak dibatasi hanya untuk FiveM. Pengguna dapat memilihnya di FiveM, Discord, browser voice chat, atau game lain.

- Receiver hanya mengirim audio ke `CABLE Input`, bukan ke speaker/headphone fisik.
- Aplikasi target memilih `CABLE Output` sebagai microphone.
- `CABLE Output` boleh dijadikan default microphone jika pengguna ingin semua aplikasi memakai sender secara otomatis.
- Receiver menyediakan tombol **Privacy Mute** yang langsung menghentikan output ke virtual cable.
- UI selalu memperingatkan saat sesi sender aktif.

Dalam konteks blueprint ini, “suara tidak bocor” berarti audio tidak diputar ke speaker/headphone, tidak direkam oleh receiver, dan tidak diarahkan ke perangkat lain. Aplikasi Windows yang sengaja memilih atau menggunakan `CABLE Output` sebagai mic dapat menerima suara; itu adalah perilaku yang diinginkan.

### Versi lanjutan

Virtual microphone driver milik sendiri dapat menghilangkan instalasi pihak ketiga, tetapi menambah kebutuhan:

- Driver signing Windows.
- Installer dan proses update driver.
- Pengujian berbagai versi Windows.
- Penanganan crash/security pada kernel boundary.

Driver sendiri bukan bagian MVP sampai kebutuhan distribusinya terbukti.

## 12. Penanganan error

Pesan harus dapat dipahami pengguna, misalnya:

- “Group belum memiliki pasangan.”
- “Password tidak cocok.”
- “Koneksi langsung gagal; mencoba relay.”
- “Virtual microphone tidak ditemukan.”
- “Mikrofon sedang digunakan atau izin ditolak.”
- “Jaringan tidak stabil; kualitas audio sedang disesuaikan.”
- “Privacy Mute aktif; audio tidak sedang dikirim ke virtual microphone.”
- “Mode tes speaker aktif dan akan berhenti otomatis.”

Aplikasi tidak boleh hanya menampilkan kode error teknis. Kode teknis tetap tersedia dalam log diagnostik lokal.

## 13. Observability dan diagnosis

Statistik runtime minimum:

- RTT/ping.
- Jitter.
- Packet loss.
- Paket terlambat/dibuang.
- Jitter buffer depth.
- Audio callback underrun/overrun.
- Bitrate codec.
- Mode direct atau relay.
- Sample rate perangkat aktual.

Pengguna melihat ringkasannya sebagai “Baik / Sedang / Buruk”; halaman diagnostik dapat menampilkan angka detail tanpa mengekspos data sensitif.

## 14. Tahapan implementasi

### Fase 0 — Feasibility dan keputusan fondasi

- Audit source, lisensi, dependency, dan build SonoBus/AOO.
- Coba koneksi custom client melalui `aoo.sonobus.net:10998` sebagai server prototipe pertama.
- Buktikan koneksi AOO antara dua Windows PC pada jaringan berbeda.
- Ukur latency, jitter, packet loss, dan stabilitas selama minimal 30 menit.
- Putuskan: core AOO baru dengan UI minimal, atau fork SonoBus yang dipangkas.

**Gate:** jangan membangun UI penuh sebelum audio Windows-to-Windows dan virtual cable terbukti bekerja.

### Fase 1 — Proof of concept Windows

- Sender Windows menangkap mic dan mengirim audio.
- Receiver Windows menerima dan menulis ke virtual audio cable.
- Group ID/password sementara.
- Statistik latency dan packet loss.
- Uji `CABLE Output` sebagai microphone di FiveM, Discord, dan satu game lain.

### Fase 2 — MVP Android

- UI sender minimal.
- Permission dan lifecycle microphone Android.
- Audio focus, AAudio/Oboe, reconnect, dan screen-lock behavior.
- Pengujian melalui Wi-Fi dan data seluler.

### Fase 3 — MVP iOS

- UI sender minimal.
- AVAudioSession, permission, interruption, route changes, dan background behavior yang diizinkan platform.
- Pengujian headset, Bluetooth, Wi-Fi, dan data seluler.

### Fase 4 — Hardening jaringan dan keamanan

- Autentikasi grup dan enkripsi end-to-end final.
- NAT traversal dan relay fallback.
- Adaptive bitrate/FEC/jitter buffer.
- Reconnect dan protocol versioning.
- Rate limiting dan sanitasi log.

### Fase 5 — Packaging

- Installer Windows dan pemeriksaan virtual cable.
- Build Android dan iOS.
- Crash reporting yang tidak mengumpulkan audio/kredensial.
- Dokumentasi penggunaan `CABLE Input/Output` pada FiveM, Discord, dan aplikasi lain.

## 15. Strategi pengujian

### Fungsional

- ID/password benar dapat terhubung.
- Password salah selalu ditolak.
- Sender kedua tidak dapat mengambil alih sesi tanpa aturan yang jelas.
- Mute tidak memutus koneksi.
- Privacy Mute menghentikan audio pada virtual microphone tanpa meninggalkan sampel lama di buffer.
- Receiver tidak menghasilkan audio pada speaker/headphone selama operasi normal.
- Device audio yang dicabut dapat dipulihkan atau menghasilkan pesan yang tepat.

### Jaringan

- Wi-Fi ke cloud PC.
- Data seluler ke cloud PC.
- NAT berbeda, CGNAT, firewall ketat, dan UDP diblokir.
- Simulasi latency 50/100/200 ms.
- Simulasi jitter 10/30/60 ms.
- Simulasi packet loss 1/3/5/10%.
- Perpindahan Wi-Fi ke data seluler.

### Audio

- Mic internal, USB, headset kabel, dan Bluetooth.
- Sample rate device berbeda.
- Bicara terus-menerus minimal 60 menit.
- Silence, suara sangat pelan, dan suara keras.
- Deteksi crackle, dropout, clipping, drift, serta buffer underrun.

### Kompatibilitas

- Windows cloud PC yang menjadi target nyata.
- FiveM, Discord, dan game lain sebagai target virtual microphone.
- Beberapa versi Android/iOS dan perangkat kelas menengah.

## 16. Kriteria penerimaan MVP

MVP dianggap selesai jika:

1. Android/iOS/Windows sender dapat mengirim mic ke Windows receiver.
2. Receiver menulis ke `CABLE Input`, lalu `CABLE Output` dapat dipilih sebagai microphone di FiveM, Discord, dan game lain.
3. Pengguna hanya perlu Group ID, password, dan tombol Connect.
4. Password salah tidak dapat menerima atau mengirim audio.
5. Direct P2P bekerja pada jaringan yang mendukungnya.
6. Koneksi pulih setelah gangguan jaringan singkat tanpa restart aplikasi.
7. Tidak ada crackle dari bug pipeline lokal dalam tes kontinu 60 menit.
8. Statistik latency, jitter, dan packet loss tersedia untuk diagnosis.
9. Semua target platform memiliki alur error permission/device yang jelas.
10. Dokumentasi instalasi dan konfigurasi virtual cable untuk FiveM, Discord, dan aplikasi lain tersedia.
11. Receiver tidak memutar audio sender ke speaker/headphone dan tidak menyimpan audio.
12. UI selalu menunjukkan saat mikrofon sender sedang ditangkap dan menyediakan Privacy Mute satu tindakan.

## 17. Risiko utama

| Risiko | Dampak | Mitigasi |
|---|---|---|
| Server publik tidak mendukung kebutuhan custom app | Pairing gagal atau tidak stabil | Validasi pada Fase 0; siapkan server sendiri |
| NAT/CGNAT memblokir P2P | Tidak dapat terhubung langsung | Relay fallback dan diagnosis koneksi |
| Jarak lintas negara tinggi | Delay voice terasa | Pilih rute/server dekat dan minimalkan buffer lokal |
| Jitter buffer terlalu agresif | Crackle/dropout | Buffer adaptif berdasarkan statistik nyata |
| Virtual cable tidak terpasang/salah dipilih | Aplikasi target tidak menerima suara | Deteksi `CABLE Input/Output` dan tampilkan panduan setup |
| Monitoring tidak sengaja aktif | Suara sender terdengar melalui speaker receiver | Nonaktif secara default, tes berbatas waktu, indikator mencolok |
| Perbedaan format audio device | Noise, speed salah, atau gagal buka device | Resampling dan negosiasi format eksplisit |
| Pembatasan lifecycle mobile | Mic berhenti saat app/background berubah | Implementasi sesuai aturan platform dan status yang jelas |
| Fork SonoBus terlalu berat dipelihara | Pengembangan melambat | Audit awal dan pisahkan shared core dari UI |

## 18. Keputusan awal

- Produk adalah **remote microphone untuk cloud gaming**, bukan aplikasi conference.
- Aliran audio MVP satu arah.
- Windows adalah platform receiver utama.
- Android, iOS, dan Windows dapat menjadi sender.
- Opus/UDP/P2P menjadi jalur media utama.
- Group ID dan password adalah satu-satunya interaksi koneksi normal.
- Virtual audio cable digunakan pada MVP.
- Receiver hanya merutekan audio ke virtual microphone; speaker monitoring nonaktif secara default.
- Semua aplikasi yang membutuhkan mic boleh memilih `CABLE Output`; tidak ada allowlist khusus FiveM.
- `aoo.sonobus.net:10998` dicoba sebagai server signaling prototipe pertama.
- AOO/SonoBus tetap melalui feasibility dan license audit sebelum dipilih sebagai fondasi final.
- Server publik digunakan untuk prototipe sampai penggunaan dan reliabilitasnya tervalidasi.

## 19. Pertanyaan yang harus dijawab pada Fase 0

1. Apakah cloud PC mengizinkan instalasi virtual audio cable?
2. Apakah cloud PC mempunyai akses UDP inbound/outbound yang cukup?
3. Apakah `aoo.sonobus.net:10998` mengizinkan dan mendukung custom client untuk skenario ini?
4. Apakah AOO menyediakan semua kebutuhan enkripsi/autentikasi, atau perlu lapisan tambahan?
5. Apakah fork SonoBus dapat didistribusikan dengan model lisensi produk yang diinginkan?
6. Apakah background microphone pada iOS/Android diperlukan, atau aplikasi selalu terbuka saat bermain?
7. Berapa wilayah utama pemain dan lokasi cloud PC untuk menentukan kebutuhan relay?
