#!/usr/bin/env bash
set -euo pipefail

ISO_PATH="${1:-build/mcsos.iso}"
LOG_PATH="${2:-build/m4-qemu-serial.log}"

mkdir -p "$(dirname "$LOG_PATH")"

timeout 15 qemu-system-x86_64 \
    -machine q35 \
    -cpu max \
    -m 256M \
    -cdrom "$ISO_PATH" \
    -boot d \
    -serial file:"$LOG_PATH" \
    -display none \
    -no-reboot \
    -no-shutdown \
    || true

echo "QEMU run selesai. Log tersimpan di: $LOG_PATH"
