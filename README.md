# Multisensor ROS 2 Workspace

Dockerized ROS 2 Humble environment for the lab's sensor stack on the Jackal:

- **Intel RealSense** D4xx · **Ximea** USB3 cameras · **FLIR Boson** thermal (V4L2)
- **Ouster OS1** LiDAR · **Xsens MTi-G** AHRS/GPS
- Optional `ros1_bridge` on the host to bring in the Jackal's ROS 1 base data

SDK dependencies are baked into the image; ROS packages live in `sensor_ws/src/` on the host and are built inside the container.

## Quick start

**First time only** (needs Docker, avahi, tmux; see [Installation](docs/installation.md)):

```bash
git clone https://github.com/hcmr-lab/jackal_multisensor_docker.git
cd jackal_multisensor_docker
chmod +x scripts/*.sh
make setup                        # ~10–20 min: .env, sensor sources, image, workspace build
./scripts/install-host-udev.sh    # once per machine, then re-plug the cameras
```

Check the interface name and `ROS_DOMAIN_ID` in [Networking](docs/networking.md) before the first launch.

**Every session** (if you haven't done the first-time setup above, do that first):

```bash
make stack         # kernel tuning (sudo) + container + ros1_bridge + sensors, all detached
make shell         # extra shell inside the container, e.g. to check rviz2
make record        # start a bag recording in ./bags (detached)
make stop-record   # stop only the recording
make stop-stack    # stop recorder, sensors and bridge (Ctrl+C / SIGINT, waits for clean exit)
```

Sessions are tmux sessions (`ms_bridge`, `ms_sensors`, `ms_record`); logs go to `logs/`.
Details, manual steps and `attach`/`status` are in [Running the stack](docs/startup.md).

## Common commands

`make help` lists everything.

| Task                      | Command                          |
|---------------------------|----------------------------------|
| Start / stop container    | `make up` · `make down`          |
| Shell in the container    | `make shell`                     |
| ros1_bridge only (host)   | `make bridge`                    |
| Rebuild workspace / image | `make rebuild` · `make build`    |
| Status / logs             | `make status` · `make logs`      |

## Documentation

| Doc | Contents |
|-----|----------|
| [Installation](docs/installation.md) | Prerequisites, first-time setup, udev, VS Code, `.env` |
| [Running the stack](docs/startup.md) | `make stack`/`record` in detail, manual launch, everyday commands |
| [Networking / CycloneDDS](docs/networking.md) | Domain ID, interface, multicast and socket buffers |
| [ROS 1 bridge](docs/ros1_bridge.md) | Bridge behavior, topics, localhost fallback |
| [Sensors and development](docs/sensors.md) | Adding a sensor package, build rules |
| [Troubleshooting](docs/troubleshooting.md) | Symptom → fix |

## Layout

```
docker/ docker-compose.yml   image and container wiring
scripts/                     setup, shell, stack, bridge, udev, apply_on_reboot
config/                      cyclonedds.xml, cyclonedds_foxy.xml, bridge_topics.yaml, rviz
sensors.repos                vcstool manifest of sensor wrappers
sensor_ws/                   src/ (your code + imported drivers), build/ install/ log/
bags/ calibrations/ logs/   data (gitignored except calibrations policy)
```

## Three rules

1. **Native Docker Engine only**: Docker Desktop breaks `network_mode: host` and X11.
2. **Never `colcon build` on the host**: use `cb`/`cbp` in the container or `make rebuild`.
3. **Never plain `rosdep install` in the container**: use `rosdep-safe-install` (it protects the custom librealsense).
