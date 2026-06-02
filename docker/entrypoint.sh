#!/bin/bash
# Entrypoint for the multisensor container.
# Sources ROS, the user workspace overlay, and argcomplete, then execs CMD.
set -e

source /opt/ros/humble/setup.bash || true  # Suppresses non-zero exit
WS_SETUP="${HOME}/ros2_ws/install/local_setup.bash"
if [ -f "${WS_SETUP}" ]; then
    source "${WS_SETUP}" || true
fi

# colcon tab-completion
if [ -f /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash ]; then
    source /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash
fi

exec "$@"