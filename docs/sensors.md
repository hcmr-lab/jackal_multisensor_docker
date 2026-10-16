# Sensors and development

Supported: Intel RealSense D4xx (RSUSB librealsense), Ximea USB3, FLIR Boson (V4L2), Ouster OS1, Xsens MTi-G.

The image bakes in the SDK dependencies. ROS packages live in `sensor_ws/src/` on the host (bind-mounted) and are built inside the container; `build/ install/ log/` are bind-mounted to `sensor_ws/` so each clone has isolated artifacts.

## Adding a sensor package

**Permanent (recommended):** add the repo and version to `sensors.repos`, then run `make setup` (downloads the code and rebuilds the image so new `package.xml` dependencies are baked in).

**Quick dev:** clone or copy the package into `sensor_ws/src/`, then:

```bash
make shell
rosdep-safe-install     # safe wrapper around rosdep install
cbp <package_name>      # colcon build --symlink-install --packages-select ... (Release)
```

## Rules

- Never plain `rosdep install` in the container: it replaces the custom RSUSB librealsense with the apt build. Use `rosdep-safe-install`.
- Never `colcon build` on the host: binaries segfault in the container. Use `cb`/`cbp` or `make rebuild`.
- Sensor drivers publish `BEST_EFFORT`; match it in subscribers.
