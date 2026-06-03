#!/usr/bin/env bash
# Drop into a shell inside the running multisensor container.
# Starts the container first if it isn't running, and refreshes the X11 cookie
# for THIS terminal's display so GUIs work whether you're on a local terminal
# or an `ssh -Y` session.
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"
cd "${PROJECT_ROOT}"

# X11 helper (provides push_xauth_into_container).
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/_x11_setup.sh"

# Check docker permission
if ! docker ps > /dev/null 2>&1; then
    echo "ERROR: Docker is not accessible. Run scripts/setup.sh for setup instructions."
    exit 1
fi

DOCKER="docker"
COMPOSE="docker compose"

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

# Refresh the cookie for this session's display, into the container's own
# ~/.Xauthority (robust across local <-> ssh -Y switches).
push_xauth_into_container "${CONTAINER_NAME}" "${USER_NAME}"

exec ${DOCKER} exec -it \
    -e DISPLAY="${DISPLAY:-}" \
    -u "${USER_NAME}" "${CONTAINER_NAME}" bash