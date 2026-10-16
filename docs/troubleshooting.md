# Troubleshooting

**`make setup` fails: container name `multisensor_container` already in use.** Another container (often the pharmarobot stack) owns the name. Check with
`docker inspect multisensor_container -f '{{index .Config.Labels "com.docker.compose.project.working_dir"}}'`.
Remove it if stale (`docker rm -f multisensor_container`), otherwise set a different `CONTAINER_NAME` in `.env`.

**RealSense not detected in the container.** Check `lsusb | grep RealSense` on the host first. If it shows there: run `./scripts/install-host-udev.sh`, check `groups | grep plugdev` in the container, try a USB 3.0 port (not a hub), re-plug.

**FLIR Boson `/dev/video0` permission denied.** Same as RealSense: `video` group and host udev rules.

**Ximea camera not detected.** Check `lsusb` on the host first; try another USB 3 port (not a hub).

**Ximea `startAcquisition()` error 13 or dropped frames.** USB buffer too small: `echo 0 | sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb` (done by `apply_on_reboot.sh`).

**Ximea topics lag by seconds, especially while recording.** Check `config/cyclonedds.xml` has `AllowMulticast` `spdp` and the 10 MB buffers, and that `apply_on_reboot.sh` was run (see [networking.md](networking.md)).

**Serial port access denied (`/dev/ttyUSB0`).** Need the `dialout` group; `setup.sh` adds the host GID inside the container.

**`librealsense2 not found` during colcon build.** Make sure the image build finished, or pass `-Dlibrealsense2_DIR=/usr/local/lib/cmake/realsense2`.

**GUIs (rviz2, rqt) won't open.** Enter via `make shell` (`./scripts/shell.sh`), which refreshes the X11 cookie for the current `$DISPLAY`. Check `echo $DISPLAY` is non-empty and that you used `ssh -Y`/`-X`.

**`docker compose` needs sudo.** `sudo usermod -aG docker $USER`, then log out/in.

**Topics missing across machines.** See the cross-machine checks in [networking.md](networking.md).

**Dropped frames / high latency on sensor topics.** Run `./scripts/apply_on_reboot.sh` (required after every reboot).

**Topic listed but no data on `echo`, or QoS warnings.** Drivers use `BEST_EFFORT`; use `--qos-reliability best_effort` or set the subscriber to match.

**`make stack` sessions.** `./scripts/stack.sh status` and `attach <name>`; logs are in `logs/`. If a stop prints a warning after 90 s, attach and inspect before killing anything.
