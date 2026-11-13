#!/usr/bin/env bash
# Source component-local settings first, then the optional deployment contract.
# The top-level robot deployment sets PHARMA_RUNTIME_ENV. Loading it last makes
# shared ROS transport values authoritative without coupling this repository to
# a fixed checkout path.

load_multisensor_runtime_env() {
    local project_root=$1 runtime_env=${PHARMA_RUNTIME_ENV:-}

    set -a
    if [[ -f "${project_root}/.env" ]]; then
        # shellcheck disable=SC1091
        source "${project_root}/.env"
    fi
    if [[ -n "${runtime_env}" ]]; then
        [[ -f "${runtime_env}" ]] || {
            echo "PHARMA_RUNTIME_ENV does not exist: ${runtime_env}" >&2
            return 1
        }
        # shellcheck disable=SC1090
        source "${runtime_env}"
    fi
    set +a

    [[ "${ROS_DOMAIN_ID:-}" =~ ^(0|[1-9][0-9]*)$ ]] \
        && (( ROS_DOMAIN_ID <= 232 )) || {
        echo "ROS_DOMAIN_ID must be set to an integer from 0 through 232." >&2
        return 1
    }
}
