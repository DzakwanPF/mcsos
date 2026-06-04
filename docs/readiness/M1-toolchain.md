# Readiness Review M1 - Toolchain Reproducible

## Identitas
- Nama mahasiswa/kelompok: [Rizqi Al & Dzakwan]
- NIM anggota: [25832073007 & 25832074004]
- Kelas: [1B]
- Dosen: Muhaemin Sidiq, S.Pd., M.Pd.
- Program Studi: Pendidikan Teknologi Informasi, Institut Pendidikan Indonesia
- Tanggal: 2026-06-04
- Commit hash: ea4ee70e85b3107e2730b66d0668b0996ddf3dd3

## Ringkasan hasil
M1 telah berhasil dijalankan dari clean checkout. Semua acceptance criteria
terpenuhi. Lingkungan build WSL 2 telah terverifikasi, toolchain freestanding
x86_64 menghasilkan ELF64 tanpa undefined symbol, QEMU dan OVMF tersedia,
dan reproducibility hash identik antara dua build. Status: siap untuk M2.

## Evidence checklist
| Evidence | Path | Status | Catatan |
|---|---|---|---|
| Toolchain versions | `build/meta/toolchain-versions.txt` | Ada | Terisi lengkap |
| Host readiness | `build/meta/host-readiness.txt` | Ada | CPU, memori, filesystem tercatat |
| QEMU capabilities | `build/meta/qemu-capabilities.txt` | Ada | q35 dan OVMF terdeteksi |
| Freestanding object | `build/proof/freestanding_probe.o` | Ada | ELF64 relocatable x86_64 |
| Freestanding ELF | `build/proof/freestanding_probe.elf` | Ada | ELF64 executable x86_64 |
| ELF header | `build/proof/readelf-header.txt` | Ada | Machine: x86-64 terverifikasi |
| ELF sections | `build/proof/readelf-sections.txt` | Ada | Section tersedia |
| Disassembly | `build/proof/objdump-disassembly.txt` | Ada | Tidak ada instruksi libc |
| Undefined symbol report | `build/proof/nm-undefined.txt` | Ada | Kosong — tidak ada undefined symbol |
| Reproducibility hash | `build/repro/sha256-run1.txt`, `build/repro/sha256-run2.txt` | Ada | Hash identik |

## Acceptance criteria M1
| Kriteria | Lulus/Gagal | Bukti |
|---|---|---|
| Repository berada di filesystem Linux WSL | Lulus | `pwd` menunjukkan `/home/dzakwanpf/src/mcsos` |
| Semua tool wajib tersedia | Lulus | `make check` berhasil tanpa ERROR |
| `make meta` berhasil | Lulus | `build/meta/toolchain-versions.txt` terisi |
| `make check` berhasil | Lulus | Semua tool OK, OVMF ditemukan |
| `make proof` berhasil | Lulus | `freestanding_probe.o` dan `.elf` terbentuk |
| `make qemu-probe` berhasil | Lulus | q35 dan OVMF terdeteksi |
| `make repro` berhasil | Lulus | Hash run1 dan run2 identik |
| `make test` berhasil dari clean checkout | Lulus | Output: `OK: M1 test suite passed` |
| `nm-undefined.txt` kosong | Lulus | File kosong, tidak ada undefined symbol |
| Hasil `readelf` menunjukkan ELF64 x86_64 | Lulus | `Machine: Advanced Micro Devices X86-64` |

## Known limitations
1. Belum ada cross GCC `x86_64-elf-gcc`; hanya menggunakan Clang/LLD.
2. Belum ada CI otomatis (GitHub Actions atau sejenisnya).
3. Belum ada hardware test pada mesin fisik x86_64.
4. Belum ada boot image; kernel belum dapat dijalankan di QEMU.
5. Reproducibility hanya diuji pada mesin yang sama; belum diuji lintas mesin.

## Risiko dan mitigasi
| Risiko | Mitigasi |
|---|---|
| Versi paket distro berubah saat `apt upgrade` dan menyebabkan build gagal | Versi toolchain dicatat di `build/meta/toolchain-versions.txt` setiap `make meta` |
| Repository dipindah ke filesystem Windows `/mnt/c` | `check_toolchain.sh` menolak path `/mnt/*` dengan status error |
| Undefined symbol masuk ke ELF proof karena flag kompilasi berubah | `proof_compile.sh` memeriksa `nm-undefined.txt` dan gagal jika tidak kosong |

## Readiness decision
- [ ] Belum siap lanjut M2.
- [ ] Siap lanjut M2 dengan catatan.
- [x] Siap lanjut M2.

Alasan keputusan: Semua target wajib M1 lulus dari clean checkout. Toolchain
freestanding x86_64 terverifikasi, ELF proof tidak memiliki undefined symbol,
QEMU dan OVMF tersedia, dan reproducibility hash identik. Lingkungan siap
untuk pembuatan boot image pada M2.
