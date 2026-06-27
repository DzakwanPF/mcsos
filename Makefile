.RECIPEPREFIX := >
SHELL := /usr/bin/env bash

BUILD_DIR    := build
KERNEL       := $(BUILD_DIR)/kernel.elf
PANIC_KERNEL := $(BUILD_DIR)/kernel.panic.elf
MAP          := $(BUILD_DIR)/kernel.map
PANIC_MAP    := $(BUILD_DIR)/kernel.panic.map
BREAKPOINT_KERNEL := $(BUILD_DIR)/kernel.breakpoint.elf
BREAKPOINT_MAP    := $(BUILD_DIR)/kernel.breakpoint.map
DISASM       := $(BUILD_DIR)/kernel.disasm.txt
SYMS         := $(BUILD_DIR)/kernel.syms.txt

CC      := clang
LD      := ld.lld
OBJDUMP := objdump
READELF := readelf
NM      := nm

COMMON_CFLAGS := --target=x86_64-unknown-none-elf -std=c17 -ffreestanding \
  -fno-builtin -fno-stack-protector -fno-stack-check -fno-pic -fno-pie \
  -fno-lto -m64 -march=x86-64 -mabi=sysv -mno-red-zone -mno-mmx \
  -mno-sse -mno-sse2 -mcmodel=kernel -Wall -Wextra -Werror \
  -Ikernel/arch/x86_64/include -Ikernel/include

CFLAGS       := $(COMMON_CFLAGS)
PANIC_CFLAGS := $(COMMON_CFLAGS) -DMCSOS_M3_TRIGGER_PANIC=1
BREAKPOINT_CFLAGS := $(COMMON_CFLAGS) -DMCSOS_M4_TRIGGER_BREAKPOINT=1
LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -T linker.ld

SRC_C    := $(shell find kernel -name '*.c' | LC_ALL=C sort)
SRC_S    := $(shell find kernel -name '*.S' | LC_ALL=C sort)

OBJ      := $(patsubst %.c,$(BUILD_DIR)/normal/%.o,$(SRC_C)) $(patsubst %.S,$(BUILD_DIR)/normal/%.o,$(SRC_S))
PANIC_OBJ := $(patsubst %.c,$(BUILD_DIR)/panic/%.o,$(SRC_C)) $(patsubst %.S,$(BUILD_DIR)/panic/%.o,$(SRC_S))
BREAKPOINT_OBJ := $(patsubst %.c,$(BUILD_DIR)/breakpoint/%.o,$(SRC_C)) $(patsubst %.S,$(BUILD_DIR)/breakpoint/%.o,$(SRC_S))

.PHONY: all build panic breakpoint inspect audit clean distclean

all: build inspect

build: $(KERNEL)

panic: $(PANIC_KERNEL)
breakpoint: $(BREAKPOINT_KERNEL)

$(BUILD_DIR)/normal/%.o: %.c
>mkdir -p $(dir $@)
>$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/normal/%.o: %.S
>mkdir -p $(dir $@)
>$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/panic/%.o: %.c
>mkdir -p $(dir $@)
>$(CC) $(PANIC_CFLAGS) -c $< -o $@

$(BUILD_DIR)/panic/%.o: %.S
>mkdir -p $(dir $@)
>$(CC) $(PANIC_CFLAGS) -c $< -o $@

$(BUILD_DIR)/breakpoint/%.o: %.c
>mkdir -p $(dir $@)
>$(CC) $(BREAKPOINT_CFLAGS) -c $< -o $@

$(BUILD_DIR)/breakpoint/%.o: %.S
>mkdir -p $(dir $@)
>$(CC) $(BREAKPOINT_CFLAGS) -c $< -o $@

$(KERNEL): $(OBJ) linker.ld
>mkdir -p $(BUILD_DIR)
>$(LD) $(LDFLAGS) -Map=$(MAP) -o $@ $(OBJ)

$(PANIC_KERNEL): $(PANIC_OBJ) linker.ld
>mkdir -p $(BUILD_DIR)
>$(LD) $(LDFLAGS) -Map=$(PANIC_MAP) -o $@ $(PANIC_OBJ)

$(BREAKPOINT_KERNEL): $(BREAKPOINT_OBJ) linker.ld
>mkdir -p $(BUILD_DIR)
>$(LD) $(LDFLAGS) -Map=$(BREAKPOINT_MAP) -o $@ $(BREAKPOINT_OBJ)

inspect: $(KERNEL)
>$(READELF) -h $(KERNEL) > $(BUILD_DIR)/kernel.readelf.header.txt
>$(READELF) -l $(KERNEL) > $(BUILD_DIR)/kernel.readelf.programs.txt
>$(NM) -n $(KERNEL) > $(SYMS)
>$(OBJDUMP) -d -Mintel $(KERNEL) > $(DISASM)
>grep -q 'ELF64' $(BUILD_DIR)/kernel.readelf.header.txt
>grep -q 'Machine:[[:space:]]*Advanced Micro Devices X86-64' $(BUILD_DIR)/kernel.readelf.header.txt
>grep -q 'kmain' $(SYMS)
>grep -q 'kernel_panic_at' $(SYMS)
>grep -q 'cpu_halt_forever' $(DISASM)

audit: inspect panic
>! $(NM) -u $(KERNEL) | grep .
>! $(NM) -u $(PANIC_KERNEL) | grep .
>grep -q 'kernel_panic_at' $(BUILD_DIR)/kernel.disasm.txt
>$(READELF) -S $(KERNEL) | grep -q '.text'
>$(READELF) -S $(KERNEL) | grep -q '.rodata'

clean:
>rm -rf $(BUILD_DIR)

distclean: clean
>rm -rf iso_root limine

LIMINE_DIR := limine
ISO_ROOT   := iso_root
ISO        := $(BUILD_DIR)/mcsos.iso

OVMF_CODE  ?= /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS  ?= /usr/share/OVMF/OVMF_VARS_4M.fd

image: build
>rm -rf $(ISO_ROOT)
>mkdir -p $(ISO_ROOT)/boot/limine
>mkdir -p $(ISO_ROOT)/EFI/BOOT
>cp $(KERNEL) $(ISO_ROOT)/boot/kernel.elf
>cp $(LIMINE_DIR)/limine-bios.sys     $(ISO_ROOT)/boot/limine/
>cp $(LIMINE_DIR)/limine-bios-cd.bin  $(ISO_ROOT)/boot/limine/
>cp $(LIMINE_DIR)/limine-uefi-cd.bin  $(ISO_ROOT)/boot/limine/
>cp $(LIMINE_DIR)/BOOTX64.EFI         $(ISO_ROOT)/EFI/BOOT/
>printf 'timeout: 3\n\n/MCSOS M4\n    protocol: limine\n    path: boot():/boot/kernel.elf\n' \
>    > $(ISO_ROOT)/boot/limine/limine.conf
>xorriso -as mkisofs \
>    -b boot/limine/limine-bios-cd.bin \
>    -no-emul-boot -boot-load-size 4 -boot-info-table \
>    --efi-boot boot/limine/limine-uefi-cd.bin \
>    --efi-boot-part --efi-boot-image \
>    --protective-msdos-label \
>    $(ISO_ROOT) -o $(ISO)
>$(LIMINE_DIR)/limine bios-install $(ISO) 2>/dev/null || true
>echo "ISO siap: $(ISO)"

# ============================================================
# M7 - VMM host unit test targets
# ============================================================
HOSTCC   ?= cc
HOST_CFLAGS_M7 := -std=c17 -Wall -Wextra -Werror -DMCSOS_HOST_TEST -Ikernel/include
VMM_OBJ  := $(BUILD_DIR)/vmm_freestanding.o

$(VMM_OBJ): kernel/core/vmm.c kernel/include/mcsos/kernel/vmm.h
>mkdir -p $(BUILD_DIR)
>$(CC) $(COMMON_CFLAGS) -c kernel/core/vmm.c -o $@

$(BUILD_DIR)/test_vmm_host: kernel/core/vmm.c tests/test_vmm_host.c kernel/include/mcsos/kernel/vmm.h
>mkdir -p $(BUILD_DIR)
>$(HOSTCC) $(HOST_CFLAGS_M7) kernel/core/vmm.c tests/test_vmm_host.c -o $@

check: $(VMM_OBJ) $(BUILD_DIR)/test_vmm_host
>$(BUILD_DIR)/test_vmm_host
>@echo "=== nm audit ==="
>nm -u $(VMM_OBJ)
>@echo "=== objdump audit ==="
>objdump -dr $(VMM_OBJ) > $(BUILD_DIR)/vmm.objdump.txt
>grep -q "invlpg" $(BUILD_DIR)/vmm.objdump.txt && echo "[OK] invlpg found"
>grep -q "cr3"    $(BUILD_DIR)/vmm.objdump.txt && echo "[OK] cr3 found"
>@echo "[PASS] M7 check selesai"

.PHONY: check
