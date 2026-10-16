#!/usr/bin/env bash
# ==============================================================================
# Run the multisensor stack in detached tmux sessions.
#
#   ./scripts/stack.sh up         kernel tuning + container + bridge + sensors
#   ./scripts/stack.sh record [name] [args...]
#                                 start `ros2 bag record` in ~/bags (detached); name becomes
#                                 the bag folder (-o name), extra args go to ros2 bag record
#   ./scripts/stack.sh status     show which sessions are running
#   ./scripts/stack.sh attach <bridge|sensors|record>   (detach: Ctrl+b d)
#   ./scripts/stack.sh stop [bridge|sensors|record|all] (default: all; Ctrl+C is
#                                 sent first so the bag is closed cleanly)
#
# Sessions: ms_bridge (host), ms_sensors, ms_record (inside the container).
# Sensor and record output is also written to logs/<name>_<timestamp>.log.
# Open RViz etc. separately with `make shell`.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"
cd "${PROJECT_ROOT}"

if [ -f .env ]; then
    set -a
    # shellcheck disable=SC1091
    source .env
    set +a
fi
CONTAINER_NAME="${CONTAINER_NAME:-multisensor_container}"
USER_NAME="${USER_NAME:-ros2user}"

LAUNCH_CMD="${SENSORS_LAUNCH_CMD:-ros2 launch multisensor_bringup multisensor.launch.py launch_realsense:=false}"
RECORD_CMD="${RECORD_CMD:-ros2 bag record -s mcap -a -x \".*(compressed|theora).*\" --max-bag-size 2147483648 --max-cache-size 209715200}"

mkdir -p logs bags
TS=$(date +%Y%m%d_%H%M%S)

have_session() { tmux has-session -t "ms_$1" 2>/dev/null; }

# Run a command inside the container as the user, log to logs/, in a tmux session.
start_in_container() {  # name, workdir, command
    local name=$1 workdir=$2 cmd=$3
    if have_session "$name"; then
        echo "-> ms_${name} already running (attach: $0 attach ${name})"; return
    fi
    local log="logs/${name}_${TS}.log"
    tmux new-session -d -s "ms_${name}" \
        "docker exec -it -w '${workdir}' -u '${USER_NAME}' '${CONTAINER_NAME}' bash -lc '${cmd//\'/\'\\\'\'}' 2>&1 | tee '${PROJECT_ROOT}/${log}'; echo; echo '[ms_${name}] exited -- press Enter to close'; read"
    echo "-> started ms_${name}  (log: ${log})"
}

ensure_container() {
    if [ -z "$(docker ps -q -f "name=^/${CONTAINER_NAME}$")" ]; then
        echo "-> Starting container..."
        docker compose up -d
        ./scripts/wait_for_ouster.sh || true
    fi
    until docker exec "${CONTAINER_NAME}" true >/dev/null 2>&1; do sleep 1; done
}

cmd_up() {
    echo "-> Kernel tuning (sudo)..."
    ./scripts/apply_on_reboot.sh
    ensure_container

    if have_session bridge; then
        echo "-> ms_bridge already running"
    else
        tmux new-session -d -s ms_bridge \
            "'${PROJECT_ROOT}/scripts/start_bridge.sh'; echo; echo '[ms_bridge] exited -- press Enter to close'; read"
        echo "-> started ms_bridge"
    fi

    start_in_container sensors "/home/${USER_NAME}" "${LAUNCH_CMD}"
    echo "Next: 'make shell' to check in RViz, then '$0 record'."
}

cmd_record() {  # [bag_name] [extra ros2 bag record args...]
    local extra=""
    if [ $# -gt 0 ] && [[ "$1" != -* ]]; then
        extra=" -o $(printf '%q' "$1")"; shift
    fi
    [ $# -gt 0 ] && extra+=" $(printf '%q ' "$@")"
    ensure_container
    start_in_container record "/home/${USER_NAME}/bags" "${RECORD_CMD}${extra}"
}

cmd_status() {
    for n in bridge sensors record; do
        if have_session "$n"; then echo "ms_$n: running"; else echo "ms_$n: -"; fi
    done
}

cmd_attach() {
    have_session "${1:?usage: $0 attach <bridge|sensors|record>}" || { echo "ms_$1 not running"; exit 1; }
    exec tmux attach -t "ms_$1"
}

session_done() { tmux capture-pane -p -t "ms_$1" 2>/dev/null | grep -q "press Enter to close"; }

# Pattern of the in-container process behind each session (for the SIGINT fallback).
proc_pattern() {
    case "$1" in
        record)  echo "ros2 bag record" ;;
        sensors) echo "ros2 launch" ;;
    esac
}

cmd_stop() {
    local targets=("${@:-all}")
    [ "${targets[0]}" = all ] && targets=(record sensors bridge)   # recorder first
    for n in "${targets[@]}"; do
        have_session "$n" || continue
        echo "-> stopping ms_$n (Ctrl+C, waiting for clean exit)..."
        tmux send-keys -t "ms_$n" C-c
        local i=0
        until session_done "$n" || ! have_session "$n"; do
            i=$((i+1))
            if [ "$i" -eq 20 ] && [ -n "$(proc_pattern "$n")" ]; then
                echo "   still running, sending SIGINT inside the container..."
                docker exec "${CONTAINER_NAME}" pkill -INT -f "$(proc_pattern "$n")" || true
            fi
            if [ "$i" -ge 90 ]; then
                echo "   WARNING: ms_$n did not exit after 90s; check '$0 attach $n'."
                continue 2
            fi
            sleep 1
        done
        have_session "$n" && tmux kill-session -t "ms_$n"
        echo "   ms_$n stopped."
    done
}

case "${1:-}" in
    up)      cmd_up ;;
    record)  shift; cmd_record "$@" ;;
    status)  cmd_status ;;
    attach)  shift; cmd_attach "$@" ;;
    stop)    shift; cmd_stop "$@" ;;
    *)       sed -n '2,17p' "$0"; exit 1 ;;
esac
