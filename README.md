# Multisensor ROS 2 Workspace

Dockerized ROS 2 Humble environment for the lab's sensor stack:

- **Intel RealSense** (D4xx series) via custom-compiled RSUSB librealsense
- **Ximea** USB3 industrial cameras
- **FLIR Boson** thermal camera (V4L2)

The image bakes in all SDK-level dependencies; ROS packages live in `src/`
on your host and are bind-mounted into the container, so you edit code with
your normal tools and rebuild inside.

---

## Quick start

```bash
git clone <this-repo> multisensor_docker
cd multisensor_docker
./scripts/setup.sh        # first run only: ~10–20 min, mostly librealsense
./scripts/shell.sh        # drop into the container
```

Inside the container:

```bash
cb                                          # alias for: colcon build --symlink-install
ros2 launch realsense2_camera rs_launch.py  # or your own launch file
```

That's it. Subsequent sessions are just `./scripts/shell.sh`.

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
| Nuke caches (keeps your src/)     | `make nuke`              |
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
│   ├── setup.sh           # First-time onboarding (idempotent)
│   └── shell.sh           # Open a shell in the container
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

### Why build artifacts go in named volumes

`build/`, `install/`, and `log/` live in Docker-managed named volumes so
they (a) persist across `docker compose down` and (b) don't end up in your
git history. If a rebuild gets confused, `make clean` wipes them.

### Adding a new sensor package

1. Add the repo to `sensors.repos`, OR drop your own package directly under `src/`.
2. If you added a new system dependency to your `package.xml`:
   - **Quick path** — run `rosdep-safe-install` inside the container.
   - **Permanent path** — `make build` to rebuild the image (re-bakes rosdep).
3. Inside the container: `cb` (or `cbp <pkg_name>` for just one).

⚠️ **Never run `rosdep install` without the `--skip-keys` flag** — it will overwrite the custom 
RSUSB librealsense build with the binary apt version, breaking everything. Always use `rosdep-safe-install` 
(the function provided in your shell) instead.
---

## Per-user setup

`scripts/setup.sh` writes a `.env` file with your host UID/GID so files
created inside the container belong to you on the host. **Don't commit it.**

If you ever need to share a machine across multiple lab members:

```bash
rm .env
./scripts/setup.sh         # regenerates .env for the new user
```

---

## VS Code users

The repo includes a `.devcontainer/devcontainer.json`. Open the project
folder, hit "Reopen in Container", and you get the full workspace with
ROS extensions, clangd, Python tooling, and a terminal already inside the
container.

---

## Troubleshooting

**RealSense camera not detected.** Check `lsusb` on the host first — if it
doesn't show up there, it won't show up in the container. Try a different
USB-3 port (not a hub). The 99-realsense-libusb.rules file is installed
into the image; on the host you may also need it for non-Docker tools.

**`librealsense2 not found` during colcon build.** Make sure the image
build completed successfully. The CMake flag
`-Dlibrealsense2_DIR=/usr/local/lib/cmake/realsense2` is set automatically
by `setup.sh`; if you build manually, include it.

**Ximea camera not detected.** Check `lsusb` on the host first — if it 
doesn't show up there, it won't show up in the container. Try a different
USB-3 port (not a hub).

**FLIR Boson `/dev/video0` permission denied.** Make sure your user is in
the `video` group on the host (`groups | grep video`); the container
inherits this via `group_add` in `docker-compose.yml`.

**GUIs (rviz2, rqt) won't open.** Run `xhost +local:docker` on the host
once per session. `scripts/shell.sh` does this for you automatically.

**`docker compose` says it needs sudo.** Either add yourself to the docker
group (`sudo usermod -aG docker $USER`, then log out/in) or let the
scripts fall back to `sudo` — they detect both.