# Networking / CycloneDDS

The stack uses `rmw_cyclonedds_cpp` with `config/cyclonedds.xml` (mounted read-write into the container). Restart the container after changing it; CycloneDDS reads the file at startup.

## 1. ROS domain ID

Set `ROS_DOMAIN_ID` in `.env` (default `0`). All machines that need to talk must use the same value.

## 2. Network interface

Edit the interface name in `config/cyclonedds.xml`:

```xml
<Interfaces>
    <NetworkInterface name="enp1s0" priority="default" multicast="default" presence_required="false"/>
</Interfaces>
```

Find yours with `ip link show`. The Jackal machine uses `enp1s0`. `auto` lets CycloneDDS choose, which can pick the wrong one on machines with several active interfaces (LAN + Wi-Fi). A wrong name silently breaks discovery.

## 3. Multicast and socket buffers

```xml
<AllowMulticast>spdp</AllowMulticast>
...
<Internal>
  <SocketReceiveBufferSize min="10MB"/>
  <SocketSendBufferSize    min="10MB"/>
</Internal>
```

- `spdp` = multicast for discovery only; data goes unicast. With `true`, large image topics (e.g. Ximea RGB) were multicast out of `enp1s0` once more than one reader was subscribed (e.g. `ros2 bag record` plus RViz). That floods the switch shared with the Ouster and the Jackal base, and the Ximea topics showed seconds of latency while recording.
- The buffers are only honored up to the host's `net.core.rmem_max` / `wmem_max`, which `scripts/apply_on_reboot.sh` raises. Run it after every reboot or the 10 MB request is capped.

## Using the topics outside this project

Copy `config/cyclonedds.xml`, install `ros-<distro>-rmw-cyclonedds-cpp`, then:

```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=0
export CYCLONEDDS_URI=file:///absolute/path/to/cyclonedds.xml
ros2 daemon stop && ros2 daemon start   # after any XML change
```

## Cross-machine checks

1. Same `ROS_DOMAIN_ID` everywhere.
2. Same interface as pinned in `config/cyclonedds.xml`.
3. `ros2 multicast receive` on one machine and `ros2 multicast send` on another.
