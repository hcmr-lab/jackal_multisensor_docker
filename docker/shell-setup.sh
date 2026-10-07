# ==============================================================================
# Interactive-shell setup for the multisensor container.
# Sourced once from ~/.bashrc (see the Dockerfile).
# ==============================================================================

# ---- ROS + workspace overlay ----
source /opt/ros/humble/setup.bash
if [ -f "${HOME}/ros2_ws/install/local_setup.bash" ]; then
    source "${HOME}/ros2_ws/install/local_setup.bash"
fi
if [ -f /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash ]; then
    source /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash
fi

# ---- Build aliases ----
# cb  : build the whole workspace.   cbp <pkg> : build a single package.
alias cb='colcon build --symlink-install --parallel-workers ${PARALLEL_WORKERS:-1} --cmake-args -DCMAKE_BUILD_TYPE=Release'
alias cbp='colcon build --symlink-install --parallel-workers ${PARALLEL_WORKERS:-1} --cmake-args -DCMAKE_BUILD_TYPE=Release --packages-select'

# ---- Safe rosdep wrapper — protects the custom RSUSB librealsense build ----
rosdep-safe-install() {
  rosdep install -i --from-path src --rosdistro humble --skip-keys='librealsense2' -y "$@"
}
