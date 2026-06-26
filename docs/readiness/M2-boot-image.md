# Readiness Review M2 - Boot Image dan Early Serial Console

## Identitas
- Proyek: MCSOS 260502
- Praktikum: M2
- Target: x86_64, QEMU, OVMF, Limine
- Nama/Kelompok: Rizqi Al & Dzakwan
- Commit hash: 
- Tanggal: 2026-06-07

## Ringkasan Status
Status yang diajukan: siap uji QEMU tahap M2.
Alasan ringkas: Build ELF, image ISO, QEMU/OVMF, dan serial marker lulus dengan evidence lengkap.

## Evidence Matrix
| Evidence | Lokasi | Status | Catatan |
|---|---|---|---|
| Preflight M2 | `build/meta/m2-preflight.txt` | PENDING | |
| Kernel ELF | `build/kernel.elf` | PENDING | |
| Kernel map | `build/kernel.map` | PENDING | |
| readelf header | `build/inspect/readelf-header.txt` | PENDING | |
| readelf PHDR | `build/inspect/readelf-program-headers.txt` | PENDING | |
| objdump | `build/inspect/objdump-disassembly.txt` | PENDING | |
| ISO | `build/mcsos.iso` | PENDING | |
| ISO checksum | `build/mcsos.iso.sha256` | PENDING | |
| Serial log | `build/qemu-serial.log` | PENDING | |
| Git commit | `build/meta/m2-commit.txt` | PENDING | |

## Invariants yang Diperiksa
1. Kernel adalah ELF64 x86_64.
2. Entry point sesuai linker script (0xffffffff80000000).
3. Kernel tidak memakai hosted libc.
4. Source dikompilasi dengan `-ffreestanding` dan `-mno-red-zone`.
5. Serial console tersedia sebelum subsistem kompleks.
6. Kernel tidak kembali setelah `kmain`.
7. Output QEMU disimpan sebagai log file.

## Failure Modes yang Diuji atau Dianalisis
| Failure mode | Pernah terjadi? | Diagnosis | Perbaikan |
|---|---|---|---|
| Toolchain salah | Tidak | | |
| OVMF tidak ditemukan | Tidak | | |
| Limine gagal fetch | Tidak | | |
| ISO gagal dibuat | Tidak | | |
| QEMU log kosong | Tidak | | |
| Entry point salah | Tidak | | |
| Reboot loop | Tidak | | |
| CRLF script | Tidak | | |

## Keputusan Readiness
- [x] Lulus M2: siap uji QEMU tahap M2.

## Catatan Reviewer
