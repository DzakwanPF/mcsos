#!/usr/bin/env bash
set -Eeuo pipefail

ISO="${1:-build/mcsos.iso}"
LOG="${2:-build/m3_serial.log}"
TIMEOUT_SEC="${MCSOS_QEMU_TIMEOUT:-15}"

fail() { echo "FAIL: $*" >&2; exit 1; }

test -f "$ISO" || fail "ISO tidak ditemukan: $ISO"
command -v qemu-system-x86_64 >/dev/null 2>&1 || fail "qemu-system-x86_64 tidak ditemukan"

mkdir -p "$(dirname "$LOG")"
rm -f "$LOG"

timeout "$TIMEOUT_SEC" qemu-system-x86_64 \
    -machine q35 \
    -m 256M \
    -smp 1 \
    -cpu qemu64 \
    -cdrom "$ISO" \
    -boot d \
    -serial file:"$LOG" \
    -no-reboot \
    -no-shutdown \
    -display none 2>&1 || true

echo "=== ISI SERIAL LOG ==="
cat "$LOG"
grep -q 'MCSOS 260502 M3 kernel entered' "$LOG" || fail "log boot M3 tidak ditemukan"
grep -q '\[M3\] selftest: basic invariants passed' "$LOG" || fail "selftest M3 tidak lulus"
echo "PASS: QEMU smoke test M3 selesai"
