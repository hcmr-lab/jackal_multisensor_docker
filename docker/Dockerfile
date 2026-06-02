# syntax=docker/dockerfile:1.6
# ==============================================================================
# STAGE 1: BUILDER — compiles Librealsense with the RSUSB backend.
# RSUSB lets us run RealSense cameras without patching the host kernel,
# which is essential for shared lab machines and Docker.
# ==============================================================================
FROM osrf/ros:humble-desktop AS builder
ARG DEBIAN_FRONTEND=noninteractive
ARG LIBREALSENSE_VERSION=v2.57.7

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        pkg-config \
        libssl-dev \
        libusb-1.0-0-dev \
        libudev-dev \
        libgtk-3-dev \
        libglfw3-dev \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        python3-dev \
    && rm -rf /var/lib/apt/lists/*

# RealSense moved their repos from IntelRealSense to realsenseai in 2025.
# Redirect may break at any time — keep the new URL.
WORKDIR /usr/src
RUN git clone --depth 1 --branch ${LIBREALSENSE_VERSION} \
        https://github.com/realsenseai/librealsense.git

RUN cd librealsense && mkdir build && cd build && cmake \
        -DCMAKE_C_FLAGS_RELEASE="${CMAKE_C_FLAGS_RELEASE} -s" \
        -DCMAKE_CXX_FLAGS_RELEASE="${CMAKE_CXX_FLAGS_RELEASE} -s" \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DFORCE_RSUSB_BACKEND=true \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_GRAPHICAL_EXAMPLES=OFF \
        -DBUILD_PYTHON_BINDINGS:bool=true \
        -DPYTHON_EXECUTABLE=/usr/bin/python3 \
        -DCMAKE_BUILD_TYPE=Release ../ \
    && make -j"$(($(nproc)-1))" \
    && make install


# ==============================================================================
# STAGE 2: RUNTIME — final image with all sensor drivers + dev tools.
# ==============================================================================
FROM osrf/ros:humble-desktop AS runner
ARG DEBIAN_FRONTEND=noninteractive
ARG USERNAME=ros2user
ARG USER_UID=1000
ARG USER_GID=1000
ARG XIMEA_URL=https://www.ximea.com/getattachment/281fd5c5-3335-4279-a494-f49c004f00c6/XIMEA_Linux_SP.tgz

# ---------- 1. System packages (runtime + dev tools + ROS extras) ----------
RUN apt-get update && apt-get install -y --no-install-recommends \
        # ROS 2 runtime extras
        ros-humble-rmw-cyclonedds-cpp \
        ros-humble-topic-tools \
        ros-humble-foxglove-bridge \
        ros-humble-image-transport \
        ros-humble-image-transport-plugins \
        ros-humble-diagnostics \
        ros-humble-cv-bridge \
        ros-humble-camera-info-manager \
        ros-humble-camera-calibration-parsers \
        # Build/dev toolchain (lab members write code in here)
        ament-cmake \
        build-essential \
        clang \
        clang-format \
        clang-tidy \
        ccls \
        gdb \
        # Hardware support
        libopencv-dev \
        libusb-1.0-0 \
        usbutils \
        udev \
        v4l-utils \
        # Quality of life
        sudo git wget curl nano vim tmux less \
        python3-pip \
        python3-colcon-common-extensions \
        python3-rosdep \
        python3-vcstool \
        python3-pytest \
    && rm -rf /var/lib/apt/lists/*

# ---------- 2. Pull in librealsense built in stage 1 ----------
COPY --from=builder /usr/local/ /usr/local/
COPY --from=builder /usr/src/librealsense/config/99-realsense-libusb.rules /etc/udev/rules.d/
ENV PYTHONPATH=/usr/local/lib${PYTHONPATH:+:${PYTHONPATH}}

# ---------- 3. Ximea SDK ----------
# The Ximea installer probes udev, which isn't running during `docker build`.
# We stub `udevadm` with `exit 0` to satisfy that check, then remove the stub.
# If this URL ever 404s, mirror the tarball internally and override XIMEA_URL.
RUN cd /tmp && \
    wget -q "${XIMEA_URL}" -O XIMEA_Linux_SP.tgz && \
    tar xzf XIMEA_Linux_SP.tgz && \
    cd package && \
    printf '#!/bin/sh\nexit 0\n' > /usr/local/bin/udevadm && \
    chmod +x /usr/local/bin/udevadm && \
    ./install -cam_usb30 && \
    rm /usr/local/bin/udevadm && \
    cd / && rm -rf /tmp/XIMEA_Linux_SP.tgz /tmp/package

# ---------- 4. User account with host-matching UID/GID ----------
# Removing 'ubuntu' frees UID 1000 — common on the osrf base image.
RUN if getent passwd ubuntu > /dev/null 2>&1; then userdel -r ubuntu; fi && \
    if ! getent group $USER_GID  > /dev/null 2>&1; then groupadd --gid $USER_GID $USERNAME; fi && \
    if ! getent passwd $USERNAME > /dev/null 2>&1; then useradd --uid $USER_UID --gid $USER_GID -m -s /bin/bash $USERNAME; fi && \
    usermod -aG plugdev,video,dialout $USERNAME && \
    echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME

RUN rosdep init || true

# HEALTHCHECK lets docker compose up --wait work reliably
HEALTHCHECK --interval=2s --timeout=1s --start-period=5s --retries=3 \
    CMD test -f /opt/ros/humble/setup.bash || exit 1
    
# ---------- 5. Entrypoint ----------
COPY docker/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# ---------- 6. Switch to user; set up shell ----------
USER $USERNAME
ENV HOME=/home/$USERNAME
WORKDIR $HOME/ros2_ws

RUN echo "source /opt/ros/humble/setup.bash"                                              >> $HOME/.bashrc && \
    echo "if [ -f \$HOME/ros2_ws/install/local_setup.bash ]; then source \$HOME/ros2_ws/install/local_setup.bash; fi" >> $HOME/.bashrc && \
    echo "source /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash"              >> $HOME/.bashrc && \
    echo "alias cb='colcon build --symlink-install'"                                      >> $HOME/.bashrc && \
    echo "alias cbp='colcon build --symlink-install --packages-select'"                   >> $HOME/.bashrc && \
    echo ""                                                                                 >> $HOME/.bashrc && \
    echo "# Safe rosdep wrapper — always protects the custom RSUSB librealsense build"    >> $HOME/.bashrc && \
    echo "rosdep-safe-install() {"                                                        >> $HOME/.bashrc && \
    echo "  rosdep install -i --from-path src --rosdistro humble --skip-keys='librealsense2' -y \"\$@\"" >> $HOME/.bashrc && \
    echo "}"                                                                               >> $HOME/.bashrc

RUN rosdep update --rosdistro=humble

# ---------- 7. Bake rosdep deps from current src/ ----------
# Copy src in only so rosdep can read every package.xml and apt-install the
# system-level dependencies they declare. Then delete src — the real src/
# arrives at runtime via the bind mount. This means first launch doesn't
# need to do a slow apt round-trip.
#
# --skip-keys=librealsense2 prevents rosdep from apt-installing librealsense
# and clobbering the custom RSUSB build we copied from stage 1.
COPY --chown=$USER_UID:$USER_GID sensor_ws/src ./src
RUN sudo apt-get update && \
    rosdep install -i --from-path src --rosdistro humble \
        --skip-keys="librealsense2" \
        -y --ignore-src && \
    sudo rm -rf /var/lib/apt/lists/* && \
    rm -rf ./src

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["/bin/bash"]