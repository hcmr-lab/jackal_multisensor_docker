# ROS 1 bridge

`ros1_bridge` (`parameter_bridge`) relays the Jackal's ROS 1 topics to ROS 2. It runs on the **host** (needs Noetic + Foxy), not in the container. Topics are listed in `config/bridge_topics.yaml` (currently `/tf`, `/odometry/filtered`, `/joint_states` from the Jackal, and `/nav2/cmd_vel` back to it).

It uses `config/cyclonedds_foxy.xml` (Foxy syntax); set the same network interface in it as in `config/cyclonedds.xml`.

```bash
make bridge        # same as ./scripts/start_bridge.sh; or `make stack` to run it detached
```

`start_bridge.sh` pings `JACKAL_IP`, waits for the ROS 1 master on port 11311, then runs `parameter_bridge` under a watchdog that restarts it if the network to the Jackal drops or the bridge crashes (it also cleans up orphaned `parameter_bridge` processes). `JACKAL_IP` and `PC_IP` can be overridden in `.env`. `ROS_DOMAIN_ID` falls back to `0`.

If the Jackal isn't reachable, the script retries every 5 s and never starts the bridge. For development without the robot, set `ALLOW_LOCALHOST_FALLBACK=true` in `.env` to use a local ROS 1 master (the watchdog is skipped then).
