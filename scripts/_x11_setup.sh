#!/usr/bin/env bash
# ==============================================================================
# X11 helper — sourced by shell.sh (and optionally setup.sh / the Makefile).
#
# Exposes push_xauth_into_container(): copy the cookie for the CURRENT $DISPLAY
# into a *running* container's own ~/.Xauthority, wildcarding the host field so
# the containerized UID is accepted.
#
# Why inject at exec-time instead of bind-mounting a cookie file?
#   xauth rewrites its file with rename(), which allocates a NEW inode. A
#   single-file Docker bind mount is pinned to the original inode, so host-side
#   cookie refreshes never reach a long-lived container. Writing the cookie
#   inside the container sidesteps that entirely and lets every session
#   (local terminal, ssh -Y, ...) bring its own per-display cookie.
# ==============================================================================

# Push the host's cookie for the current $DISPLAY into a running container.
#   $1 = container name
#   $2 = username inside the container
push_xauth_into_container() {
    local container="$1"
    local user="$2"

    # No X session, or no xauth on the host → nothing to do.
    [ -n "${DISPLAY:-}" ] || return 0
    command -v xauth > /dev/null 2>&1 || return 0

    local source_auth="${XAUTHORITY:-$HOME/.Xauthority}"
    [ -f "${source_auth}" ] || return 0

    # nlist "$DISPLAY" selects only this display's entry (xauth normalises
    # localhost / <host>/unix / :N to one entry, so it matches an ssh -Y cookie
    # too). sed rewrites the family field to ffff (FamilyWild) so the cookie is
    # accepted regardless of host identity.
    xauth -f "${source_auth}" nlist "${DISPLAY}" 2>/dev/null \
        | sed -e 's/^..../ffff/' \
        | docker exec -i -u "${user}" "${container}" \
              bash -c 'xauth -f "$HOME/.Xauthority" nmerge - 2>/dev/null' || true
}