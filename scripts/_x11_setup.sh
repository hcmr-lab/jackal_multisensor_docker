#!/usr/bin/env bash
# ==============================================================================
# X11 setup helper — sourced by setup.sh and shell.sh.
# Always sets XAUTH to a stable per-user path so the container's bind mount
# doesn't change between sessions. Generates an actual cookie only when DISPLAY
# is set (i.e. when we're in an X session).
# ==============================================================================

# 1. Always pin XAUTH to the same path so the bind mount is consistent
XAUTH="/tmp/.docker-xauth-$(id -u)"
export XAUTH

# 2. Ensure the file exists so docker doesn't auto-create it as a directory
touch "$XAUTH"
chmod 0644 "$XAUTH"

# 3. No X session → nothing to write. Bind mount points to empty file.
if [ -z "${DISPLAY:-}" ]; then
    return 0 2>/dev/null || exit 0
fi

# 4. Determine the source xauth file. Priority:
#   1. $XAUTHORITY (if set AND non-empty AND file exists)
#   2. ~/.Xauthority (the universal fallback)
# Note: we don't trust XAUTHORITY's default-when-unset behavior because
# some shells set it to empty string, which xauth handles inconsistently.
SOURCE_AUTH=""
if [ -n "${XAUTHORITY:-}" ] && [ -f "${XAUTHORITY}" ]; then
    SOURCE_AUTH="${XAUTHORITY}"
elif [ -f "${HOME}/.Xauthority" ]; then
    SOURCE_AUTH="${HOME}/.Xauthority"
fi

if [ -n "${SOURCE_AUTH}" ] && command -v xauth > /dev/null 2>&1; then
    : > "$XAUTH"  # truncate
    xauth -f "${SOURCE_AUTH}" nlist 2>/dev/null \
        | sed -e 's/^..../ffff/' \
        | xauth -f "$XAUTH" nmerge - 2>/dev/null || true
fi

# 6. Belt-and-suspenders fallback
if command -v xhost > /dev/null 2>&1; then
    xhost +SI:localuser:"$(id -un)" > /dev/null 2>&1 || true
fi