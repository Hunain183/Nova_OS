#!/bin/sh
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root_dir"

require_command() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "error: $1 is required for make verify" >&2
        exit 1
    }
}

require_command clang
require_command ld
require_command make
require_command xorriso
require_command qemu-system-x86_64

make kernel
make iso
test -s build/NovaOS.iso

tmp_dir=$(mktemp -d)
trap 'kill "$qemu_pid" 2>/dev/null || true; rm -rf "$tmp_dir"' EXIT INT TERM

serial_log="$tmp_dir/serial.log"
qemu-system-x86_64 \
    -accel tcg \
    -cdrom build/NovaOS.iso \
    -display none \
    -serial "file:$serial_log" &
qemu_pid=$!

sleep 2
grep -q 'NovaOS: serial online' "$serial_log"
grep -q 'NovaOS: interrupts online' "$serial_log"

kill "$qemu_pid"
wait "$qemu_pid" 2>/dev/null || true
echo "NovaOS verification passed: kernel, ISO, and boot initialization"
