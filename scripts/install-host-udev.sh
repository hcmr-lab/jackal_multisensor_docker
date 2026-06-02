#!/usr/bin/env bash
# ==============================================================================
# Install host-side udev rules for RealSense devices.
# Run ONCE per lab machine (by an admin or any user with sudo), not per user.
# This copies the rules from inside the Docker image to the host.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"
cd "${PROJECT_ROOT}"

# Get the image name from docker-compose.yml
IMAGE_NAME=$(grep -E '^\s+image:' docker-compose.yml | head -1 | awk '{print $2}')
if [ -z "$IMAGE_NAME" ]; then
    echo "ERROR: Could not find image name in docker-compose.yml"
    exit 1
fi

echo "========================================================================"
echo "  Installing host udev rules for RealSense"
echo "  Image: $IMAGE_NAME"
echo "========================================================================"
echo ""

# Check if the image exists; if not, build it
if ! docker image inspect "$IMAGE_NAME" > /dev/null 2>&1; then
    echo "-> Image '$IMAGE_NAME' not found locally. Building..."
    docker compose build
    echo ""
fi

RULES_FILE="/etc/udev/rules.d/99-realsense-libusb.rules"

echo "-> Extracting rules from container image..."
docker run --rm "$IMAGE_NAME" cat /etc/udev/rules.d/99-realsense-libusb.rules | \
    sudo tee "$RULES_FILE" > /dev/null
echo "   Wrote to $RULES_FILE"

echo ""
echo "-> Setting permissions..."
sudo chmod 0644 "$RULES_FILE"

echo ""
echo "-> Reloading udev rules and triggering device re-enumeration..."
sudo udevadm control --reload-rules
sudo udevadm trigger

echo ""
echo "========================================================================"
echo "  Done."
echo ""
echo "  Next steps:"
echo "  1. Make sure your user is in the 'plugdev' group:"
echo "     groups | grep plugdev"
echo "     If not, run: sudo usermod -aG plugdev \$USER"
echo "     Then log out and back in."
echo ""
echo "  2. Unplug and re-plug your RealSense camera."
echo ""
echo "  3. Verify it's detected:"
echo "     lsusb | grep RealSense"
echo "========================================================================"