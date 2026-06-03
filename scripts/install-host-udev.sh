#!/usr/bin/env bash
# ==============================================================================
# Install host-side udev rules for USB sensors (RealSense, Ximea).
# Run ONCE per lab machine (by an admin or any user with sudo), not per user.
# Copies the sensor udev rules baked into the Docker image out to the host.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"
cd "${PROJECT_ROOT}"

if ! docker ps > /dev/null 2>&1; then
    echo "ERROR: Docker is not accessible to your user."
    echo "       Add yourself to the docker group:"
    echo "         sudo usermod -aG docker \$USER"
    echo "         newgrp docker"
    exit 1
fi

DOCKER="docker"
COMPOSE="docker compose"

# Get the image name from compose
set -a
[ -f .env ] && source .env
set +a

IMAGE_NAME=$(grep -E '^\s+image:' docker-compose.yml | head -1 | awk '{print $2}')
IMAGE_NAME=$(eval echo "$IMAGE_NAME")

# Check if image is built; if not, guide the user
if ! ${DOCKER} image inspect "$IMAGE_NAME" > /dev/null 2>&1; then
    cat <<EOF
ERROR: Docker image '$IMAGE_NAME' not found.

The Docker image must be built before extracting udev rules.

Please run setup.sh first:

  ./scripts/setup.sh

This will:
  1. Generate your .env file with your UID/GID
  2. Build the Docker image  
  3. Import sensor packages
  4. Build the ROS workspace

After setup.sh completes, run this script again:

  ./scripts/install-host-udev.sh

EOF
    exit 1
fi

echo "========================================================================"
echo "  Installing host udev rules for USB sensors (RealSense, Ximea)"
echo "  Image: $IMAGE_NAME"
echo "========================================================================"
echo ""

# ---- 1. Discover sensor udev rules baked into the image --------------------
# RealSense ships 99-realsense-libusb.rules; the Ximea SDK installer drops its
# own rule whose filename varies between SDK versions, so we glob for it rather
# than hardcode. --entrypoint bash keeps ROS-sourcing output out of stdout.
echo "-> Discovering sensor udev rules inside the image..."
RULE_FILES=$(${DOCKER} run --rm --entrypoint bash "$IMAGE_NAME" -c '
    shopt -s nullglob
    for f in /etc/udev/rules.d/*realsense* \
         /etc/udev/rules.d/*ximea*; do
      echo "$f"
    done')

if [ -z "${RULE_FILES//[[:space:]]/}" ]; then
    echo "   WARNING: no RealSense/Ximea udev rules found in the image."
    echo "            Is the image fully built and did the Ximea SDK install succeed?"
fi
 
# ---- 2. Copy each discovered rule out to the host --------------------------
while IFS= read -r src; do
    [ -z "$src" ] && continue
    base="$(basename "$src")"
    dest="/etc/udev/rules.d/${base}"
    echo "-> Installing ${base}"
    ${DOCKER} run --rm --entrypoint cat "$IMAGE_NAME" "$src" | sudo tee "$dest" > /dev/null
    sudo chmod 0644 "$dest"
done <<< "$RULE_FILES"

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
echo "  2. Unplug and re-plug your camera(s)."
echo ""
echo "  3. Verify detection:"
echo "     lsusb | grep -Ei 'realsense|ximea'"
echo "========================================================================"