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
echo "  Installing host udev rules for RealSense"
echo "  Image: $IMAGE_NAME"
echo "========================================================================"
echo ""

RULES_FILE="/etc/udev/rules.d/99-realsense-libusb.rules"

echo "-> Extracting rules from container image..."
${DOCKER} run --rm "$IMAGE_NAME" cat /etc/udev/rules.d/99-realsense-libusb.rules | \
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