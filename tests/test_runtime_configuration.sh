#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
temporary="$(mktemp -d)"
trap 'rm -rf "${temporary}"' EXIT

mkdir -p "${temporary}/project"
printf 'ROS_DOMAIN_ID=0\nRMW_IMPLEMENTATION=local\n' > "${temporary}/project/.env"
printf 'ROS_DOMAIN_ID=7\nRMW_IMPLEMENTATION=shared\n' > "${temporary}/runtime.env"

# shellcheck disable=SC1091
source "${root}/scripts/load-runtime-env.sh"
PHARMA_RUNTIME_ENV="${temporary}/runtime.env"
load_multisensor_runtime_env "${temporary}/project"

[[ "${ROS_DOMAIN_ID}" == 7 ]]
[[ "${RMW_IMPLEMENTATION}" == shared ]]

printf 'ROS_DOMAIN_ID=999\n' > "${temporary}/bad.env"
PHARMA_RUNTIME_ENV="${temporary}/bad.env"
if load_multisensor_runtime_env "${temporary}/project" 2>/dev/null; then
    echo "invalid ROS_DOMAIN_ID was accepted" >&2
    exit 1
fi

echo "multisensor runtime configuration tests passed"
