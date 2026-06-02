#!/usr/bin/env bash
# Drop into a shell inside the running multisensor container.
# Starts the container first if it isn't running.
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"
cd "${PROJECT_ROOT}"

# Set up X11 forwarding BEFORE we figure out sudo, so XAUTH is in the env
# we then pass through to sudo (or use directly).
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/_x11_setup.sh"

# Docker permission detection — preserve XAUTH/HOME/DISPLAY through sudo
# so docker compose sees the right values when it expands ${XAUTH}.
if docker ps > /dev/null 2>&1; then
    DOCKER="docker"
    COMPOSE="docker compose"
elif sudo -n docker ps > /dev/null 2>&1; then
    DOCKER="sudo --preserve-env=XAUTH,HOME,DISPLAY docker"
    COMPOSE="sudo --preserve-env=XAUTH,HOME,DISPLAY docker compose"
else
    echo "ERROR: docker not runnable as you or via passwordless sudo."
    exit 1
fi

# Load .env to know the container name
if [ -f .env ]; then
    set -a
    # shellcheck disable=SC1091
    source .env
    set +a
fi

CONTAINER_NAME="${CONTAINER_NAME:-multisensor_container}"
USER_NAME="${USER_NAME:-ros2user}"

if [ -z "$(${DOCKER} ps -q -f name=${CONTAINER_NAME})" ]; then
    echo "-> Container not running, starting it..."
    ${COMPOSE} up -d
    until ${DOCKER} exec "${CONTAINER_NAME}" true > /dev/null 2>&1; do
        sleep 1
    done
fi

exec ${DOCKER} exec -it -u "${USER_NAME}" "${CONTAINER_NAME}" bash