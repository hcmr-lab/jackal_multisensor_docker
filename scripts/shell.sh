#!/usr/bin/env bash
# Drop into a shell inside the running multisensor container.
# Starts the container first if it isn't running.
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"
cd "${PROJECT_ROOT}"

if docker ps > /dev/null 2>&1; then
    DOCKER="docker"
    COMPOSE="docker compose"
else
    DOCKER="sudo docker"
    COMPOSE="sudo docker compose"
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

# Allow X11 forwarding from the container (host-local only, no network exposure)
xhost +local:docker > /dev/null 2>&1 || true

if [ -z "$(${DOCKER} ps -q -f name=${CONTAINER_NAME})" ]; then
    echo "-> Container not running, starting it..."
    ${COMPOSE} up -d
    until ${DOCKER} exec "${CONTAINER_NAME}" true > /dev/null 2>&1; do
        sleep 1
    done
fi

exec ${DOCKER} exec -it -u "${USER_NAME}" "${CONTAINER_NAME}" bash