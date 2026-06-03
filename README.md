# Multisensor ROS 2 Workspace

Dockerized ROS 2 Humble environment for the lab's sensor stack:

- **Intel RealSense** (D4xx series) via custom-compiled RSUSB librealsense
- **Ximea** USB3 industrial cameras
- **FLIR Boson** thermal camera (V4L2)

The image bakes in all SDK-level dependencies; ROS packages live in `src/`
on your host and are bind-mounted into the container, so you edit code with
your normal tools and rebuild inside.

---
## Prerequisites

- Linux host (Ubuntu 20.04 tested)
- [Docker Engine](https://docs.docker.com/engine/install/ubuntu/) & [Docker Compose](https://docs.docker.com/compose/install/)
- Your user in the `docker` group:

  ```bash
    sudo usermod -aG docker $USER
    newgrp docker     # activate in current shell (or log out/in)
    docker ps         # verify
  ```
- **Avahi daemon** (for Ouster mDNS hostname discovery):

  ```bash
    sudo apt install avahi-daemon
    sudo systemctl start avahi-daemon
    sudo systemctl enable avahi-daemon  # auto-start on reboot
  ```

## Quick start

```bash
git clone https://github.com/hcmr-lab/jackal_multisensor_docker.git
cd jackal_multisensor_docker
chmod +x scripts/*.sh
./scripts/setup.sh        # first run only: ~10–20 min, mostly librealsense
./scripts/shell.sh        # drop into the container
```

### Host udev rules (one-time setup)
 
**RealSense, Ximea, and other USB cameras won't work without host-side udev rules.** This is a one-time setup per lab machine.
 
```bash
./scripts/install-host-udev.sh
```
 
This script extracts the RealSense and Ximea udev rules from the Docker image and installs them on your host. After running it, **unplug and re-plug your camera**. 
 
Then verify on the host:
```bash
lsusb | grep RealSense
```

Inside the container:

```bash
cb                                          # alias for: colcon build --symlink-install
ros2 launch realsense2_camera rs_launch.py  # or your own launch file
```

That's it. Subsequent sessions are just `./scripts/shell.sh`.

### Advanced: host-side USB access

You only need this if you want to access cameras **directly on the host** 
(not through the container). The Docker container does not require host-user 
group membership — its USB access is handled by `group_add` in 
`docker-compose.yml`.

```bash
sudo usermod -aG plugdev,video,dialout $USER
newgrp plugdev
```

---

## Common tasks

| Task                              | Command                  |
|-----------------------------------|--------------------------|
| First-time setup                  | `make setup`             |
| Start container                   | `make up`                |
| Open a shell                      | `make shell`             |
| Stop container                    | `make down`              |
| Rebuild the Docker image          | `make build`             |
| Rebuild the colcon workspace      | `make rebuild`           |
| Clear build/install/log           | `make clean`             |
| Show what's running               | `make status`            |

---

## What goes where

```
multisensor_docker/
├── docker/
│   ├── Dockerfile         # Two-stage: builder (librealsense) + runner
│   └── entrypoint.sh      # Sources ROS + overlay + argcomplete
├── docker-compose.yml     # Hardware mounts, named volumes, env wiring
├── scripts/
│   ├── setup.sh          # First-time onboarding (idempotent; detects host GIDs)
│   ├── install-host-udev.sh  # One-time: extracts udev rules to host
│   ├──_x11_setup.sh      # Sourced by shell.sh; injects X11 cookie into the container
│   └── shell.sh          # Open a shell in the container
├── config/
│   └── cyclonedds.xml     # DDS tuning (mounted read-only into container)
├── sensors.repos          # vcstool manifest of sensor wrapper packages            
├── sensor_ws/
│   ├── src/               # ROS 2 packages (your code + vcs-imported wrappers)
│   ├── build/
│   ├── install/
│   └── log/
├── bags/                  # rosbag2 recordings (gitignored)
├── maps/                  # SLAM maps (gitignored)
├── calibrations/          # Camera intrinsics, extrinsics
├── .env                   # Per-user config (generated, gitignored)
└── .devcontainer/         # VS Code "Reopen in Container" support
```

### Why `src/` is bind-mounted

So you can edit code in VS Code / Neovim / whatever on the host and have
changes appear instantly in the container. The image does **not** contain
any ROS packages — it contains only the SDK-level pieces (librealsense,
Ximea SDK) plus apt-installed rosdep dependencies.

### Why build artifacts are bind-mounted

`build/`, `install/`, and `log/` are bind-mounted to `./sensor_ws/` so 
each user's clone has isolated artifacts. 

### Adding a new sensor package

1. Add the repo to `sensors.repos`, OR drop your own package directly under `src/`.
2. If you added a new system dependency to your `package.xml`:
   - **Quick path** — run `rosdep-safe-install` inside the container.
   - **Permanent path** — `make build` to rebuild the image (re-bakes rosdep).
3. Inside the container: `cb` (or `cbp <pkg_name>` for just one).

⚠️ **Never run `rosdep install` without the `--skip-keys` flag** — it will overwrite the custom 
RSUSB librealsense build with the binary apt version, breaking everything. Always use `rosdep-safe-install` 
(the function provided in your shell) instead.

⚠️ Never run colcon build on the host. Always build inside the container
(cb alias, or make rebuild). Host-built binaries will segfault when loaded
by the container
---

## Per-user setup

`scripts/setup.sh` writes a `.env` file with:
- Your host UID/GID (so files created inside belong to you)
- Detected group IDs for `plugdev`, `video`, `dialout` (so the container user 
  can access USB devices and serial ports)
 
**Don't commit `.env`.** If you move to a different machine with different group 
assignments, just delete `.env` and re-run `scripts/setup.sh`.

---

## VS Code users
⚠️ run `scripts/setup.sh` first

The repo includes a `.devcontainer/devcontainer.json`. Open the project
folder, hit "Reopen in Container", and you get the full workspace with
ROS extensions, clangd, Python tooling, and a terminal already inside the
container.

---

## Troubleshooting

**RealSense not detected inside container:**
```bash
# First, check if it's detected on the HOST:
lsusb | grep RealSense
 
# If it shows up on host but not in container:
# 1. Make sure you installed host udev rules:
./scripts/install-host-udev.sh
 
# 2. Inside container make sure your user is in plugdev group:
groups | grep plugdev
 
# 3. Try a different USB 3.0 port (not a hub). Unplug and re-plug.
```
**FLIR Boson `/dev/video0` permission denied:**
Similar to RealSense — make sure you're in the `video` group and host udev rules are installed.

**Ximea camera not detected.** Check `lsusb` on the host first — if it 
doesn't show up there, it won't show up in the container. Try a different
USB-3 port (not a hub).
 
**Serial port access denied (e.g., `/dev/ttyUSB0`):**
Make sure you're in the `dialout` group. `setup.sh` auto-detects your host's `dialout` GID and adds you to it inside the container.

**`librealsense2 not found` during colcon build.** Make sure the image
build completed successfully. You can try including the CMake flag
`-Dlibrealsense2_DIR=/usr/local/lib/cmake/realsense2` like in `setup.sh`.

**Ximea `startAcquisition()` fails with error 13 (or dropped frames).** The Linux default USB buffer (16 MB) is too small for Ximea USB3 cameras. run 
  ```bash
  echo 0 | sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb
  ```

**GUIs (rviz2, rqt) won't open.** Always enter the container with
`./scripts/shell.sh` — it forwards your current terminal's `$DISPLAY` and
refreshes the X11 cookie *inside* the container for that display. Because the
cookie is refreshed per session, this works even when you start the container
from one terminal (e.g. a local VS Code terminal) and later open a GUI from a
separate `ssh -Y` session on the same machine. If a GUI still fails, check that
`echo $DISPLAY` is non-empty in the terminal you ran `shell.sh` from, and that
you connected with `ssh -Y` (or `-X`).

**`docker compose` says it needs sudo.** Add yourself to the docker
group (`sudo usermod -aG docker $USER`, then log out/in).