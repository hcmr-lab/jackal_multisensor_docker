#!/usr/bin/env bash
# scripts/apply_on_reboot.sh
#
# Increases kernel network buffer sizes and removes the USB memory cap.
# Required for reliable LiDAR (Ouster) and USB camera (RealSense, Ximea) data.
#
# These are NOT persisted on purpose — they reset on reboot, which avoids
# conflicts with other software that may have different needs.
#
# Run this once after each boot before starting the sensors:
#   ./scripts/apply_on_reboot.sh
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    exec sudo "$0" "$@"
fi

sysctl -w net.core.rmem_max=33554432
sysctl -w net.core.rmem_default=33554432
sysctl -w net.core.wmem_max=33554432
sysctl -w net.core.wmem_default=33554432
sh -c 'echo 0 > /sys/module/usbcore/parameters/usbfs_memory_mb'

echo "Kernel tuning applied (resets on next reboot)."