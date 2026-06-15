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
    export $(grep -v '^#' "$PROJECT_ROOT/.env" | xargs)
fi

# Fallback values if not in .env
ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-7}

# --- 3. Source ROS Distros ---
# Note: ros1_bridge typically requires Noetic and Foxy on the host
source /opt/ros/noetic/setup.bash
source /opt/ros/foxy/setup.bash

# --- 4. Auto-Detect Jackal Network ---
JACKAL_IP="169.254.126.137"
PC_IP="169.254.179.150"

echo "Pinging Jackal Base at $JACKAL_IP..."
if ping -c 1 -W 1 "$JACKAL_IP" &> /dev/null; then
    echo "[SUCCESS] Jackal base detected! Routing ROS 1 to external master."
    export ROS_MASTER_URI=http://$JACKAL_IP:11311
    export ROS_IP=$PC_IP
else
    echo "[INFO] Jackal base unreachable. Routing ROS 1 to local master."
    export ROS_MASTER_URI=http://localhost:11311
    export ROS_IP=127.0.0.1
fi

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

# --- 6. Execution ---
echo "Flushing ROS 2 Daemon cache..."
ros2 daemon stop > /dev/null 2>&1 || true

# Load the topics whitelist into the ROS 1 Parameter Server
if [ -f "$CONFIG_DIR/bridge_topics.yaml" ]; then
    echo "Loading bridge topics from $CONFIG_DIR/bridge_topics.yaml..."
    rosparam load "$CONFIG_DIR/bridge_topics.yaml"
else
    echo "Error: bridge_topics.yaml not found in $CONFIG_DIR"
    exit 1
fi

echo "Bridge Active. Waiting for matched subscribers..."
# Using parameter_bridge is safer as it only bridges topics defined in your YAML
ros2 run ros1_bridge parameter_bridge

#ros2 run ros1_bridge dynamic_bridge --bridge-all-topics
