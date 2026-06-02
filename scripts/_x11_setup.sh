#!/usr/bin/env bash
# ==============================================================================
# X11 setup helper — sourced by setup.sh and shell.sh before invoking
# docker compose. Sets the XAUTH env var to a stable cookie file at a
# predictable path under /tmp, so the container always knows where to mount
# the credentials from — regardless of:
#   - Wayland/GDM's random /run/user/.../mutter-Xwaylandauth.XXXXX paths
#   - sudo stripping HOME and XAUTHORITY from the env
#
# Sourcing (not exec'ing) is intentional: we need `export` to affect the
# caller's environment so the subsequent `docker compose` call inherits it.
# ==============================================================================

# No X session → nothing to do. Use `return` since we're sourced.
if [ -z "${DISPLAY:-}" ]; then
    return 0 2>/dev/null || exit 0
fi

# Per-user cookie path. Stable across sessions on the same machine.
XAUTH="/tmp/.docker-xauth-$(id -u)"
export XAUTH

# Build the cookie file. The `sed 's/^..../ffff/'` trick rewrites the
# family field of each entry to "Wild" (ffff), which makes the cookie
# accept connections from any UID — including the containerized one.
if command -v xauth > /dev/null 2>&1; then
    rm -f "$XAUTH"
    touch "$XAUTH"
    xauth nlist "$DISPLAY" 2>/dev/null \
        | sed -e 's/^..../ffff/' \
        | xauth -f "$XAUTH" nmerge - 2>/dev/null || true
    chmod 0644 "$XAUTH"
fi

# Belt-and-suspenders fallback: if the cookie merge failed (e.g. xauth not
# installed, no cookies for $DISPLAY), allow local connections from the
# current user. `+local:docker` was incorrect — :docker is interpreted as
# a username and the container connects as the user's UID, not "docker".
if command -v xhost > /dev/null 2>&1; then
    xhost +SI:localuser:"$(id -un)" > /dev/null 2>&1 || true
fi