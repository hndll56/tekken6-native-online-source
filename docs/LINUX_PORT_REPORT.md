# Laporan implementasi dan validasi Linux

Tanggal: 21 Oktober 2026. Target: Linux x86-64, Qt 6, SDL, Vulkan, ELF; tanpa Wine/Proton.

## Kesimpulan yang didukung bukti

Implementasi port disertakan sebagai source nyata, sistem build, patch SDK dan pengujian. Launcher, GGPO, alat ISO, SDK runtime, code generator dan plugin Vulkan telah dibangun di lingkungan Linux. **Belum ada bukti executable game Tekken 6 lengkap berhasil dibangun atau game dapat dimainkan.** Tidak tersedia ISO/XEX legal milik pemain untuk menjalankan codegen dan link final game.

Repositori URL awal dan ZIP lampiran hanya membawa README. Source sebenarnya ditautkan di README, dan diambil dari `Ekspym/tekken6-native-online-source` pada commit `e61f084be5d062c96389b8e8cb9fc4dc5fedaed4`. SDK dipertahankan pada gitlink upstream `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`; patch Windows asli tetap ada dan ditambah `0003-linux-host.patch`.

## Audit ketergantungan Windows dan penggantinya

| Bagian/fungsi sebenarnya | Ketergantungan Windows asal | Implementasi Linux |
| --- | --- | --- |
| Launcher dan resource build | `/W3`, `/Zi`, linker `/DEBUG`, `.rc`, library `dwmapi/winhttp/dxgi/winmm/dbghelp/ws2_32` tanpa kondisi | `launcher/CMakeLists.txt` memisahkan sumber/link/flag Windows dan Linux; Qt Widgets/Network/Multimedia, SDL2, Vulkan |
| Entry point, single instance, crash | Win32 mutex, DbgHelp, SEH, `MessageBox`, DWM dark title | `main_linux.cpp`: `QLockFile` per package+instance di runtime directory, exception reporting ke stderr; fault/core dump native pada game |
| HTTP update dan SOAP router | WinHTTP | `http_qt.cpp`: `QNetworkAccessManager`, event loop per worker, proxy, redirect, timeout, validasi CA TLS, batas respons lokal 1 MiB |
| Koneksi server/lobi | Qt sockets sudah lintas platform; sertifikat hanya resource resmi | Protokol dipertahankan; `T6_LOBBY_CERT` menambah jalur pin yang bisa disediakan pemain/pengelola tanpa menonaktifkan verifikasi |
| SSDP/UPnP | Winsock startup, SOCKET, DWORD receive timeout, panjang `int` | Socket POSIX, `timeval`, `socklen_t`, waktu monotonic Qt; SOAP dan pemetaan port asli tetap berjalan |
| Transport dan polling GGPO | Winsock, `WaitForMultipleObjects`, event handle, `timeGetTime`, CRT Microsoft, `DllMain` | `platform_linux.*`, UDP nonblocking POSIX, `poll(2)`, `steady_clock`, helper CRT berbatas; tidak mengganti algoritme rollback/prediction |
| Identitas dan region | Registry MachineGuid, Windows timezone | `QSysInfo::machineUniqueId()` di-hash dengan skema upstream; pemetaan region Qt dipertahankan |
| CPU/GPU | `__cpuid` MSVC, DXGI adapter | CPUID GCC/Clang, enumerasi physical device Vulkan dan argumen `--vulkan_device`; daftar bukan sertifikasi dukungan semua fitur GPU |
| Bunyi match found | MCI, PlaySound | `QMediaPlayer` + `QAudioOutput`, file kustom dan fallback suara bawaan |
| Gamepad/stick pada launcher | WinMM, XInput DLL dinamis | SDL2 game controller/raw joystick; binding format lama dipertahankan |
| Gamepad/stick pada game | WinMM SDK Windows-only | Driver mapping asli memakai adapter raw SDL3 di Linux; gamepad standar tetap backend SDL3 SDK |
| Keyboard game | `GetAsyncKeyState`, foreground HWND | Event SDL3, latch penekanan pendek, fokus game, konversi keymap Qt ke identifier tombol upstream; tidak membaca keycode X11 mentah sebagai virtual key Windows |
| Proses dan launcher crash | Windows Job Object, Toolhelp snapshot, `TerminateProcess` | `QProcess::setChildProcessModifier` memasang `PR_SET_PDEATHSIG` sebelum exec, disertai cek race kematian parent; tidak membunuh proses lain berdasarkan nama |
| Jendela game | EnumWindows/HWND, show/hide, fullscreen style mutation | Perintah per PID dan thread UI SDL game; dapat bekerja melalui model surface Wayland tanpa kontrol jendela lintas proses |
| Timer dan scheduling runtime | QueryPerformanceCounter, GetTickCount, Sleep, thread ID | Monotonic nanosecond timer C++, scheduler sleep/yield, Linux TID; eksperimen prioritas memakai `setpriority` dengan pelaporan kegagalan |
| Snapshot/diagnostik memori | VirtualQuery, VirtualProtect | `/proc/self/maps` dan `mprotect`; hole/noaccess tidak diperlakukan sebagai memori aman untuk dibaca |
| File status dan profile | MoveFileEx, CopyFile, `.exe`, `.dll`, path paket writable | Rename POSIX atomik, filesystem copy eksklusif, path UTF-8, nama ELF/`.so`, XDG data/config/cache, executable bits saat staging |
| Runtime hook lookup | GetModuleHandle/GetProcAddress pada DLL | `dlopen(RTLD_NOLOAD)`/`dlsym` pada runtime/plugin yang sudah dimuat; tidak memuat runtime kedua |
| Pengambilan sample profiler | SuspendThread/GetThreadContext/DbgHelp | Sample RIP dengan sinyal real-time dan ucontext, klasifikasi fungsi guest/module melalui `dladdr`; belum diuji saat game berjalan |
| Research page watch | Vectored exception/access violation/single-step | Handler SIGSEGV/SIGTRAP, mprotect dan trap flag; record diagnostik diproses di luar signal handler; belum diuji terhadap game |
| Hook audio/semaphore milik proyek di SDK | `__declspec(dllexport)`, NtQuerySemaphore, QPC tanpa guard | Export ELF ber-visibility default, query semaphore melalui mutex internal POSIX, timer portable; algoritme audio lockstep dipertahankan |
| Rendering dan present count | Direct3D12/ROV dan counter D3D12 | Backend Vulkan SDK, FSI bila tersedia/fallback framebuffer, export counter swap Vulkan; cache pipeline D3D12 tidak disalin sebagai pipeline Vulkan |
| Builder pemain | llvm-mingw/clang++.exe, rexglue.exe, xdvdfs.exe, objek/prebuilt DLL Windows | `build_linux.py` membangun SDK/ELF dari source dan game dari salinan pemain, menerapkan semua patch generated code, menyimpan output di direktori data pemain |
| Update paket | tar.exe, nama patch Windows | bsdtar dan aset khusus `tekken6_native_online_linux_x86_64_patch.zip`; source publik belum menyediakan release Linux untuk uji update live |
| Skrip pengembangan | `.cmd`, `.bat`, PowerShell, portable_launcher Win32 | Tetap tersedia untuk Windows; alur produksi Linux menggunakan CMake/Python/Bash. Skrip diagnostik PowerShell lama belum seluruhnya diterjemahkan satu per satu |

Nama `XamInputGetState`, `XInputGetCapabilities` dan simbol `__imp__...` dalam hook adalah **ABI guest Xbox 360**. Itu tidak diganti dengan API Linux langsung: SDK tetap menerjemahkan input host ke struktur guest. Demikian juga struktur/offset perangkat grafis guest yang dipakai snapshot bukan objek host Direct3D, sehingga offset game tidak diubah sekadar karena renderer host menjadi Vulkan.

## Bukti build dan uji yang benar-benar dijalankan

Lingkungan: Ubuntu 24.04.3 amd64; GCC 13 untuk launcher/komponen; Qt 6.4.2, SDL2 2.30; Clang 20.1.2 + libstdc++ 13 untuk SDK; SDK SDL3/Vulkan dari submodule yang dikunci. Dependensi diekstrak ke prefix lokal karena instalasi paket sistem tidak dapat ditulis di lingkungan eksekusi. Recipe README memakai instalasi paket distro biasa.

| Pemeriksaan | Hasil dan cakupan |
| --- | --- |
| Build komponen CMake/Ninja | Lulus: launcher ELF, `libggpo.a`, `xdvdfs_tool`, executable test |
| Build SDK | Lulus: ELF `rexglue`, `librexruntime.so`, `librexgpu-xenos.so` dengan backend Vulkan; warning upstream tetap ada |
| CLI code generator | `rexglue --help` berhasil, exit 0 |
| Simbol runtime/plugin | `nm -D` menemukan audio lockstep/advance, current guest context, semaphore count, global lock, hold object, dan swap count pada `.so` |
| Runtime C++ | 35/35 unit tulisan tangan lulus `-fsyntax-only`; tidak memakai stub guest untuk memperoleh hasil ini. `tekken6_app.cpp` kini hanya memerlukan deklarasi image config yang memang disediakan output codegen |
| `ggpo_loopback_rollback` | Lulus: dua proses, UDP loopback sungguhan, struktur input 12 byte, 240 frame, rollback load terjadi, checksum akhir sama |
| `linux_host_services` | Lulus: monotonic timer, mmap/query/protect, rename atomik dengan nama UTF-8, copy eksklusif, lookup simbol |
| `qt_http_tls` | Lulus: GET, POST/error SOAP, redirect, batas body, worker thread, TLS local trusted CA, penolakan sertifikat tidak dipercaya |
| `linux_child_lifecycle` | Lulus: parent launcher dimatikan SIGKILL; child terkena SIGKILL melalui parent-death signal dan direap test |
| Smoke launcher | Buka/tutup otomatis offscreen exit 0; tidak menguji layout visual compositor asli. Placeholder art upstream menghasilkan warning painter offscreen |
| Patch SDK dari checkout bersih | Patch asli + Linux + FFmpeg berhasil diterapkan; pemanggilan kedua mengenali keadaan sudah diterapkan |
| Builder input salah | Path hilang dan folder tanpa Default.xex ditolak dengan status 1; tidak melewati pemeriksaan hash game |

Ringkasan CTest terakhir: **4/4 lulus**. Log yang relevan ada di `docs/validation/`.

## Belum diuji / masih terhambat

1. **Codegen dan link final Tekken 6:** memerlukan XEX/ISO yang cocok dengan hash pada README. Karena tidak ada, tidak ada binary game final dalam deliverable ini. Langkah berikutnya: jalankan builder dengan salinan milik pemain, lalu perbaiki bila codegen/link menghasilkan masalah tambahan.
2. **Gameplay dan determinisme game sebenarnya:** belum ada sesi game, uji snapshot game, audio rollback, rematch, practice, custom character atau soak test. Test checksum sintetis membuktikan port transport/rollback GGPO bekerja; bukan bukti determinisme Tekken 6 pada Linux.
3. **Lobi resmi:** server, endpoint dan sertifikat produksi tidak dipublikasikan upstream. Diperlukan konfigurasi/pin dari pengelola atau backend lobi yang kompatibel. Tidak dibuat server pengganti yang berpura-pura mendukung ranked/matchmaking resmi.
4. **GPU:** build Vulkan berhasil, tetapi belum ada rendering Tekken, uji Intel N100, pengukuran FPS, atau verifikasi FSI/framebuffer pada hardware. Pemilihan GPU otomatis pada UI bersifat informasi; daftar device bukan hasil uji kelayakan renderer.
5. **Perangkat input/audio dan desktop:** belum diuji dengan controller/stick fisik, perangkat audio, X11/Wayland nyata, fokus compositor atau multi-monitor. Latch keyboard dan raw binding terimplementasi tetapi belum diuji lewat game.
6. **Internet/router:** NAT traversal, UPnP router fisik, packet loss tinggi, latency internet, putus-sambung dan reconnect belum diuji.
7. **Updater:** jalur Linux dan nama aset terpisah diimplementasikan; instalasi patch Linux live dan restart sesudah update belum diuji karena tidak ada release Linux upstream. Jalankan launcher melalui `tools/run_linux.sh` agar executable berada di direktori update yang writable.
8. **Diagnostik sinyal/profiler:** pemeriksaan kompilasi lulus; validasi saat game berjalan masih diperlukan. Research watch sangat sensitif terhadap concurrency seperti implementasi asal. Core dump Linux tetap menjadi jalur debugger native.
9. **Reproducibility:** commit SDK/submodule dan flag dikunci; timestamp dan prefix path builder distabilkan. Byte identik antar dua mesin belum diuji. Distribusikan satu build Linux yang sama untuk matchmaking, bukan menganggap build distro berbeda kompatibel.
10. **Windows dan distro lain:** dukungan source Windows dipertahankan tetapi build/regresi Windows belum dijalankan. Perintah Arch/EndeavourOS dan Debian disediakan; lingkungan validasi ini Ubuntu saja.

## Langkah penerimaan berikutnya pada PC pemain

Ikuti README: build komponen dan jalankan CTest, bootstrap SDK, lalu build dari ISO. Sesudah game boot, jalankan dua instance Linux dari paket yang sama dengan `--instance 1` dan `--instance 2`, konfigurasi server yang kompatibel, periksa hash/snapshot lewat alat determinisme upstream, lalu lakukan pertandingan LAN/internet dan uji rematch/practice/audio. Hasil itu diperlukan sebelum memberi label siap main pada port ini.
