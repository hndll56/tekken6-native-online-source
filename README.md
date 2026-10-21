# Tekken 6 Native Online — port Linux

Implementasi native **Linux x86-64**, memakai Qt 6, SDL dan Vulkan. Tidak memakai Wine/Proton atau toolchain Windows. Dukungan Windows dipertahankan melalui kode per platform.

**Status:** launcher, GGPO, alat ISO, code generator, runtime SDK, dan plugin Vulkan berhasil dibangun di Linux. Ke-35 unit C++ runtime tulisan tangan lolos pemeriksaan kompilasi. **Executable game lengkap belum dibangun/dijalankan:** ISO Tekken 6 yang dibutuhkan tidak tersedia pada pekerjaan ini. Gameplay, determinisme game sebenarnya, dan pertandingan online resmi belum terverifikasi. Lihat [hasil dan batas pengujian](docs/LINUX_PORT_REPORT.md).

ZIP awal `Tekken6-native-online-main.zip` hanya berisi README. Implementasi ini berasal dari repositori source yang ditautkan README: [Ekspym/tekken6-native-online-source](https://github.com/Ekspym/tekken6-native-online-source), commit `e61f084be5d062c96389b8e8cb9fc4dc5fedaed4`. Tidak ada kode hasil recompilation, ISO, data game, atau save pemain dalam arsip ini.

## Dependensi

CPU harus mendukung x86-64-v2. SDK memerlukan Clang dengan C++23; **Clang 20 + libstdc++ 13** dipakai pada validasi. Clang 18 dengan libstdc++ 13 gagal pada `std::expected`; libc++ 18 juga belum mencukupi implementasi floating-point `from_chars` yang dipakai SDK.

Arch Linux / EndeavourOS:

```bash
sudo pacman -S --needed base-devel clang cmake ninja git python \
  qt6-base qt6-multimedia sdl2 vulkan-headers vulkan-icd-loader \
  libx11 libxext libxrandr libxcursor libxi libxinerama libxtst libxss \
  libxkbcommon libxkbcommon-x11 libxcb xcb-util-keysyms \
  wayland wayland-protocols libdecor alsa-lib libpulse dbus nasm openssl libarchive
```

Pasang driver Vulkan sesuai GPU. Untuk Intel, termasuk Intel N100:

```bash
sudo pacman -S --needed mesa vulkan-intel
```

Ubuntu 24.04 dengan repositori updates/universe aktif:

```bash
sudo apt update
sudo apt install build-essential clang-20 cmake ninja-build git python3 pkg-config \
  qt6-base-dev qt6-multimedia-dev qt6-qpa-plugins libsdl2-dev libvulkan-dev \
  libx11-dev libx11-xcb-dev libxext-dev libxrandr-dev libxcursor-dev \
  libxi-dev libxinerama-dev libxtst-dev libxss-dev libxkbcommon-dev \
  libxkbcommon-x11-dev libxcb1-dev libxcb-keysyms1-dev \
  libwayland-dev wayland-protocols libdecor-0-dev libasound2-dev \
  libpulse-dev libdbus-1-dev libibus-1.0-dev nasm openssl libarchive-tools
export CC=clang-20 CXX=clang++-20
```

Paket Debian umumnya sama; gunakan versi Clang yang menyediakan C++23 lengkap bersama standard library yang sesuai. Perintah instalasi Arch/Debian belum dijalankan pada distro tersebut. Untuk Intel/AMD di Ubuntu/Debian, pasang `mesa-vulkan-drivers` bila driver Vulkan belum tersedia.

## Build launcher dan jalankan pengujian

Jalankan dari direktori source hasil ekstraksi. Langkah ini tidak membutuhkan ISO atau SDK.

```bash
cmake --preset linux-components
cmake --build --preset linux-components
ctest --preset linux-components
bash tools/run_linux.sh --no-game
```

Script menjalankan launcher native dan menyediakan lokasi executable yang dapat diperbarui oleh updater. Sesudah mengubah/membangun ulang launcher, pakai `bash tools/run_linux.sh --install --no-game` untuk memasang hasil build terbaru.

Tanpa display, smoke test dapat dijalankan dengan:

```bash
QT_QPA_PLATFORM=offscreen ./build/components/launcher/t6-native-launcher \
  --no-game --smoke-test-ms 1200
```

Ini menguji startup/shutdown; pengujian tersebut tidak membuktikan tampilan pada X11/Wayland atau output GPU game.

## Siapkan SDK dan bangun game milikmu

SDK dikunci pada commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`; dependency turunannya mengikuti gitlink SDK. Script berikut bekerja dari clone Git maupun arsip source ZIP:

```bash
python3 tools/bootstrap_linux.py
```

Script mengambil submodule dan menerapkan patch asli + patch Linux. Ia menolak revision SDK yang salah dan perubahan lokal yang tidak cocok dengan rangkaian patch. Sumber dependensi memerlukan koneksi internet. Tidak ada pengunduhan game.

Siapkan **Tekken 6 Xbox 360 USA/Europe Rev 1** dari salinanmu sendiri. SHA-256 `Default.xex` yang diterima tetap memakai pemeriksaan upstream:

```text
ee138a4fe8d32a0edc131451377f4377041a49626153fa992db24f5420a7f15c
```

```bash
# Arch: compiler clang/clang++ bawaan paket clang.
# Ubuntu: ekspor CC=clang-20 CXX=clang++-20 seperti di atas.
python3 builder/build_linux.py --check '/path/Tekken 6.iso'
T6_JOBS=2 python3 builder/build_linux.py '/path/Tekken 6.iso'
bash tools/run_linux.sh
```

Alternatif: buka **Profile → BUILD GAME** pada launcher dan pilih ISO/folder disc hasil ekstraksi. Proses ini mengompilasi SDK native, menerjemahkan XEX, menerapkan keempat patch netplay, lalu mengompilasi game dan menyalin `.so` yang diperlukan. `T6_JOBS` default 2 agar tidak terlalu boros RAM.

Jalankan perintah build secara berurutan: konfigurasi, build, lalu test. `ctest` tidak akan menemukan executable jika langkah build gagal. Ganti `/path/Tekken 6.iso` dengan path ISO atau folder game yang benar; path contoh tersebut memang akan ditolak.

Setelah berhasil, output berada di `$XDG_DATA_HOME/tekken6-native-online/native/tekken6`, atau `~/.local/share/tekken6-native-online/native/tekken6` bila variabel tersebut tidak disetel. Builder hanya mencetak `DONE` setelah executable benar-benar ada. Game juga bisa dijalankan langsung:

```bash
app_home="${T6_HOME:-${XDG_DATA_HOME:-$HOME/.local/share}/tekken6-native-online}"
"$app_home/native/tekken6" --game_data_root="$app_home/native/game"
```

Pembaruan memakai data yang sudah diekstrak:

```bash
python3 builder/build_linux.py --update
```

`--relink` juga diterima, tetapi recipe Linux membangun dari source; ia tidak memakai DLL/objek prebuilt Windows dan dapat menjalankan lebih banyak pekerjaan daripada relink paket Windows.

## Online

GGPO rollback, format input 12 byte, matchmaking client, NAT punch, UPnP, lobi, ranked/casual dan practice tetap ada. Uji jaringan memakai GGPO asli dengan dua proses dan socket UDP, bukan backend palsu.

**Server resmi, alamat produksinya, serta sertifikat pin resminya tidak dipublikasikan oleh upstream.** Source publik memakai placeholder. Untuk terhubung, minta endpoint dan sertifikat yang benar kepada pengelola, lalu:

```bash
T6_LOBBY_SERVER='tls://hostname:7700' \
T6_LOBBY_CERT='/path/server-cert.pem' \
  bash tools/run_linux.sh
```

Sertifikat diverifikasi; port ini tidak mengabaikan error TLS. Server lobi yang kompatibel tetap diperlukan dan tidak disertakan. Koneksi lobi resmi, matchmaking sebenarnya dan NAT pada router fisik belum diuji.

Kedua pemain perlu memakai **build Linux yang sama**. Hash executable/runtime tetap diperiksa; kompatibilitas dengan pemain Windows atau build dari toolchain berbeda tidak dijanjikan. Recipe source dikunci dalam `DEPENDENCIES.lock.json`, timestamp distabilkan melalui `SOURCE_DATE_EPOCH`, dan path build dipetakan. Byte identik antar-distro/toolchain belum diverifikasi.

## Data, kontrol, dan grafis

- Game, profile, status dan log: `${XDG_DATA_HOME:-~/.local/share}/tekken6-native-online`.
- Konfigurasi: `${XDG_CONFIG_HOME:-~/.config}/tekken6-native-online/launcher_N.ini`.
- Shader cache: `${XDG_CACHE_HOME:-~/.cache}/tekken6-native-online/<run>`.
- `T6_HOME=/disk/lain/tekken6` memindahkan data/build besar. `T6_SOURCE_DIR` menunjuk checkout source; `tools/run_linux.sh` mengaturnya otomatis.
- Keyboard menggunakan event SDL; keymap yang sama tetap memakai identifier tombol numerik upstream. Joystick/stick mentah memakai SDL dengan format binding lama; gamepad standar memakai SDL gamepad.
- Launcher mengirim perintah jendela ke proses game; operasi show/hide/fullscreen dilakukan di thread UI SDL agar sesuai X11/Wayland.
- Vulkan memilih fragment shader interlock bila didukung; fallback memakai framebuffer. Kebenaran gambar/performa kedua jalur pada Tekken 6 belum diuji. Cache pipeline Direct3D tidak dipakai sebagai cache Vulkan.
- Hotkey lintas aplikasi dari `T6_NET_KEYS_GLOBAL` tidak tersedia pada Wayland melalui event SDL; input yang tersedia berasal dari jendela game. Kebijakan fokus compositor juga dapat membatasi permintaan raise/focus.

## Validasi runtime tanpa ISO

Untuk membangun SDK/codegen/plugin secara terpisah:

```bash
# Arch / EndeavourOS (Ubuntu: pakai clang-20 / clang++-20):
export CC=clang CXX=clang++
cmake -S third_party/rexglue-sdk -B build/sdk-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_CXX_FLAGS=-march=x86-64-v2 -DREXGLUE_USE_VULKAN=ON \
  -DREXGLUE_USE_D3D12=OFF -DREXGLUE_ENABLE_TRACY=OFF
cmake --build build/sdk-linux --target rexglue rexgpu-xenos --parallel 2
python3 tools/check_linux_runtime.py --compiler "$CXX"
```

Ekspor `CC` dan `CXX` **sebelum** perintah tersebut (`export CC=clang CXX=clang++` di Arch, `export CC=clang-20 CXX=clang++-20` di Ubuntu). Script syntax-check tidak menggantikan build/link dan uji game dari ISO.

## Windows dan dokumentasi

Entry point, WinHTTP, WinMM/XInput, Job Objects, Direct3D dan skrip build Windows tetap disediakan. Build Windows belum diuji ulang. Panduan asli ada di [docs/README-upstream.md](docs/README-upstream.md); audit pemetaan platform serta batasan rinci ada di [docs/LINUX_PORT_REPORT.md](docs/LINUX_PORT_REPORT.md).

Kredit dan lisensi dependensi mengikuti upstream: ReXGlue (BSD 3-Clause), GGPO (MIT), font Bai Jamjuree (SIL OFL), Material Symbols (Apache 2.0). Tekken adalah merek Bandai Namco; proyek ini tidak berafiliasi dengan Bandai Namco.
