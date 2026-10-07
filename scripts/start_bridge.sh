#!/bin/bash
set -e

echo "Initializing Cross-Distro Bridge (Noetic -> Humble via CycloneDDS)..."

# --- 1. Path Discovery ---
# Directory where this script lives
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECT_ROOT=$(dirname "$SCRIPT_DIR")
CONFIG_DIR="$PROJECT_ROOT/config"

# --- 2. Load Environment Variables ---
if [ -f "$PROJECT_ROOT/.env" ]; then
    echo "Loading configuration from .env..."
    set -a
    source "$PROJECT_ROOT/.env"
    set +a
fi

# Fallback values if not in .env
ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-7}

# --- 3. Source ROS Distros ---
# Note: ros1_bridge typically requires Noetic and Foxy on the host
source /opt/ros/noetic/setup.bash
source /opt/ros/foxy/setup.bash

# --- 4. Auto-Detect Jackal Network ---
JACKAL_IP="${JACKAL_IP:-169.254.126.137}"
PC_IP="${PC_IP:-169.254.179.150}"
# Opt-in: if the Jackal is unreachable, route ROS 1 to a local master instead
# of retrying forever. Off by default -- set ALLOW_LOCALHOST_FALLBACK=true in
# .env for dev/testing without the physical robot attached.
ALLOW_LOCALHOST_FALLBACK="${ALLOW_LOCALHOST_FALLBACK:-false}"

# --- 5. CycloneDDS & Middleware Setup ---
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=$ROS_DOMAIN_ID

# Point ONLY the bridge to the Foxy-specific XML
if [ -f "$CONFIG_DIR/cyclonedds_foxy.xml" ]; then
    export CYCLONEDDS_URI="file://$CONFIG_DIR/cyclonedds_foxy.xml"
    echo "Using Foxy CycloneDDS config: $CYCLONEDDS_URI"
else
    echo "Warning: cyclonedds_foxy.xml not found!"
fi

echo "Flushing ROS 2 Daemon cache..."
ros2 daemon stop > /dev/null 2>&1 || true

# --- 6. Execution Loop ---
# Trap Ctrl+C to exit the script AND kill any background jobs (the bridge)
trap "echo -e '\n[INFO] Ctrl+C detected. Exiting bridge script...'; kill \$(jobs -p) 2>/dev/null; exit 0" SIGINT SIGTERM

# Clean up any orphaned bridge processes from previous crashed sessions
echo "Checking for old bridge processes..."
pkill -9 -f parameter_bridge 2>/dev/null || true

while true; do
    echo "--------------------------------------------------------"
    echo "Pinging Jackal Base at $JACKAL_IP..."

    JACKAL_REACHABLE=false
    if ping -c 1 -W 1 "$JACKAL_IP" &> /dev/null; then
        JACKAL_REACHABLE=true
    fi

    if [ "$JACKAL_REACHABLE" = true ]; then
        echo "[SUCCESS] Jackal network detected! Routing ROS 1 to external master."
        export ROS_MASTER_URI=http://$JACKAL_IP:11311
        export ROS_IP=$PC_IP

        # When the Jackal first turns on, the ping succeeds
        # instantly, but roscore takes a few extra seconds to start.
        echo "Waiting for ROS 1 Master on port 11311 to come online..."
        while ! timeout 2 bash -c "echo > /dev/tcp/$JACKAL_IP/11311" 2> /dev/null; do
            sleep 2
        done
        echo "[SUCCESS] ROS 1 Master is fully ready!"
    elif [ "$ALLOW_LOCALHOST_FALLBACK" = "true" ]; then
        echo "[INFO] Jackal base unreachable. ALLOW_LOCALHOST_FALLBACK=true -- routing ROS 1 to local master."
        export ROS_MASTER_URI=http://localhost:11311
        export ROS_IP=127.0.0.1
    else
        echo "[INFO] Jackal base unreachable. Waiting 5 seconds before retrying..."
        sleep 5
        continue
    fi

    # Load the topics whitelist into the ROS 1 Parameter Server
    if [ -f "$CONFIG_DIR/bridge_topics.yaml" ]; then
        echo "Loading bridge topics from $CONFIG_DIR/bridge_topics.yaml..."
        rosparam load "$CONFIG_DIR/bridge_topics.yaml"
    else
        echo "Error: bridge_topics.yaml not found in $CONFIG_DIR"
        exit 1
    fi

    echo "Bridge Active. Starting parameter_bridge..."

    # Run the bridge in the BACKGROUND (using &) and save its Process ID
    ros2 run ros1_bridge parameter_bridge &
    BRIDGE_PID=$!

    # Watchdog Loop
    while kill -0 $BRIDGE_PID 2>/dev/null; do

        # Check the physical network. If it drops, the Jackal is off/rebooting.
        # Only meaningful when we're actually bridging against the Jackal --
        # a localhost-fallback run has no physical link to lose.
        if [ "$JACKAL_REACHABLE" = true ] && ! ping -c 1 -W 2 "$JACKAL_IP" &> /dev/null; then
            echo "[WARNING] Lost network connection to Jackal! Killing frozen bridge..."
            kill -9 $BRIDGE_PID 2>/dev/null || true
            break
        fi

        sleep 30
    done

    echo "Bridge stopped. Restarting in 5 seconds..."
    sleep 5
done
