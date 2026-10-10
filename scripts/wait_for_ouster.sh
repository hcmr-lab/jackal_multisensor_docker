#!/usr/bin/env bash
# Wait for the Ouster's mDNS hostname to resolve before the sensor launch
# fires. On a fresh container start there's a brief window where the Ouster
# hasn't been (re)discovered yet -- ros2 launch's one resolution attempt can
# land inside that window and ouster_ros's attempt_reconnect only retries the
# connection, not the hostname lookup, so it stays stuck until something
# restarts it. This closes that race by polling on the host, which already
# has a live NSS/mDNS path to the sensor, before `make up` hands off to
# `docker compose exec ... ros2 launch`.
set -uo pipefail

HOSTNAME_TO_RESOLVE="${OUSTER_HOSTNAME:-os-122212000760.local}"
TIMEOUT="${OUSTER_WAIT_TIMEOUT:-10}"

if ! command -v avahi-resolve >/dev/null 2>&1; then
    echo "wait_for_ouster: avahi-resolve not found, skipping mDNS wait." >&2
    exit 0
fi

elapsed=0
while (( elapsed < TIMEOUT )); do
    if resolved="$(avahi-resolve -4 -n "${HOSTNAME_TO_RESOLVE}" 2>/dev/null)"; then
        echo "wait_for_ouster: ${resolved}"
        exit 0
    fi
    sleep 1
    elapsed=$((elapsed + 1))
done

echo "wait_for_ouster: ${HOSTNAME_TO_RESOLVE} did not resolve within ${TIMEOUT}s;" \
     "continuing anyway, ouster_ros will keep retrying the connection." >&2
exit 0
