# Installation

## Prerequisites

- Linux host (Ubuntu 20.04 tested)
- [Docker Engine](https://docs.docker.com/engine/install/ubuntu/) & [Docker Compose](https://docs.docker.com/compose/install/) (native Engine, not Docker Desktop)
- Your user in the `docker` group:

  ```bash
  sudo usermod -aG docker $USER
  newgrp docker     # activate in current shell (or log out/in)
  docker ps         # verify
  ```
- **Avahi daemon** (for Ouster mDNS hostname discovery):

  ```bash
  sudo apt install avahi-daemon
  sudo systemctl enable --now avahi-daemon
  ```
- **tmux** (for `make stack`, see [startup.md](startup.md)): `sudo apt install tmux`
- Only if you use the ros1_bridge: ROS 1 Noetic and ROS 2 Foxy installed on the host.

## First-time setup

```bash
git clone https://github.com/hcmr-lab/jackal_multisensor_docker.git
cd jackal_multisensor_docker
chmod +x scripts/*.sh
make setup        # ~10–20 min the first time, mostly librealsense
```

`make setup` (`scripts/setup.sh`) is idempotent. It generates `.env`, imports the
sensor wrappers from `sensors.repos`, builds the image, starts the container and
builds the colcon workspace.

Then, once per machine:

```bash
./scripts/install-host-udev.sh    # RealSense/Ximea udev rules on the host
```

After it runs, **unplug and re-plug the cameras**, and verify on the host with
`lsusb | grep RealSense`.

Before the first launch also check the [networking settings](networking.md)
(`ROS_DOMAIN_ID`, network interface name).

### Host-side USB access (optional)

Only needed to use cameras directly on the host. The container does not need it
(access is handled by `group_add` in `docker-compose.yml`).

```bash
sudo usermod -aG plugdev,video,dialout $USER
newgrp plugdev
```

### VS Code

Run `make setup` first. The repo includes `.devcontainer/devcontainer.json`: open
the folder and choose "Reopen in Container".

## Per-user `.env`

`scripts/setup.sh` writes `.env` with:
- Your host UID/GID (files created inside the container belong to you)
- Detected group IDs for `plugdev`, `video`, `dialout`
- `ROS_DOMAIN_ID`, `RMW_IMPLEMENTATION`, `CYCLONEDDS_URI`
- `JACKAL_IP` / `PC_IP` for `start_bridge.sh`, and `ALLOW_LOCALHOST_FALLBACK` (off by default)
- `PARALLEL_WORKERS` / `BUILD_JOBS` for colcon (lower them if a build runs out of memory)
- `SENSORS_LAUNCH_CMD` and `RECORD_CMD`, the commands `scripts/stack.sh` runs (the launch defaults to `launch_realsense:=false`)

**Don't commit `.env`.** On a machine with different group assignments, delete it
and re-run `make setup`.
