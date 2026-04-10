#!/usr/bin/env bash
# baSic_ QEMU launch script
# Usage: ./scripts/run.sh <image.img> [debug]


IMAGE="${1:-build/baSic_.img}"
MODE="${2:-}"

if [ ! -f "$IMAGE" ]; then
    echo "[ERROR] Image not found: $IMAGE"
    echo "Run 'make' first."
    exit 1
fi

QEMU_ARGS=(
    -drive "format=raw,file=$IMAGE,index=0,media=disk" # boot from raw disk image
    -m 32M                                               # 32 MB RAM (like early Linux)
    -no-reboot                                          # stop instead of rebooting on triple fault
    -no-shutdown
)

if [ "$MODE" = "debug" ]; then
    # Serial output to terminal + GDB stub on port 1234
    QEMU_ARGS+=(
        -serial stdio
        -s                           # open gdbserver on tcp::1234
        -S                           # freeze CPU at startup (wait for GDB)
    )
    echo "[INFO] GDB stub enabled on port 1234"
    echo "[INFO] In another terminal: gdb -ex 'target remote :1234'"
fi

echo "[RUN] qemu-system-i386 ${QEMU_ARGS[*]}"
exec qemu-system-i386 "${QEMU_ARGS[@]}"