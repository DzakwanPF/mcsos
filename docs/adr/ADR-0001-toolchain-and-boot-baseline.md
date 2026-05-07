# ADR-0001 — Toolchain dan Boot Baseline MCSOS 260502

## Status
Accepted for M0 baseline.

## Context
MCSOS dikembangkan pada host Windows 11 x64, tetapi targetnya adalah bare metal x86_64. Program kernel tidak boleh bergantung pada ABI Windows atau Linux host. Lingkungan build harus dapat direproduksi oleh mahasiswa, asisten, dan dosen.

## Decision
1. Build environment utama adalah WSL 2 Linux environment.
2. Repository utama ditempatkan di filesystem Linux WSL, bukan `/mnt/c`.
3. Toolchain awal M0 memakai paket distro: Clang/LLVM, LLD, binutils, NASM, Make, CMake, Ninja, Python 3.
4. Smoke test M0 memakai `clang --target=x86_64-unknown-none` untuk menghasilkan object freestanding.
5. Emulator utama untuk milestone berikutnya adalah QEMU system x86_64.
6. Firmware emulator adalah OVMF untuk jalur UEFI.
7. Bootloader awal yang direkomendasikan adalah Limine karena mendukung x86-64 dan protokol boot modern; keputusan final akan divalidasi pada M1/M2.
8. GCC `x86_64-elf` dari source bersifat opsional pada M0 kecuali ditetapkan wajib oleh dosen.

## Consequences

### Keuntungan
- Setup lebih seragam di Windows 11.
- Toolchain Linux tersedia melalui package manager.
- Workflow QEMU + GDB selaras dengan praktik OS development.
- Struktur evidence dapat direproduksi.

### Trade-off
- WSL 2 memiliki boundary VM yang perlu dipahami.
- Akselerasi KVM di WSL bisa terbatas; TCG menjadi baseline (lebih lambat tapi stabil).
- Versi paket distro bisa berbeda antar mesin, sehingga metadata versi wajib dicatat.

## Review Trigger
ADR ini harus ditinjau ulang jika:
1. Target arsitektur berubah dari x86_64.
2. Distro WSL distandarkan ulang.
3. Bootloader diganti dari Limine ke GRUB atau custom UEFI loader.
4. Toolchain utama diganti dari LLVM ke GCC-only atau sebaliknya.
5. CI resmi proyek diperkenalkan.

