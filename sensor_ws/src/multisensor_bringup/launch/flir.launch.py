import subprocess
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def get_flir_port():
    """Auto-discovers the FLIR Boson /dev/videoX port."""
    try:
        # Run v4l2-ctl to list all video devices
        result = subprocess.check_output(['v4l2-ctl', '--list-devices'], text=True)
        lines = result.split('\n')
        
        # Look for the Boson/FLIR name in the output
        for i, line in enumerate(lines):
            if 'Boson' in line or 'FLIR' in line:
                # The actual /dev/videoX path is on the next line
                return lines[i+1].strip()
    except Exception as e:
        print(f"Warning: Failed to auto-detect FLIR. {e}")
        
    # Fallback if not found
    return '/dev/video0'


def generate_launch_description():
    """Launch the FLIR Boson USB camera driver."""

    # --- PREFIXED TO PREVENT COLLISION ---
    namespace_arg = DeclareLaunchArgument(
        'flir_namespace', default_value='flir_boson',
        description='The ROS namespace for the camera node'
    )
    frame_id_arg = DeclareLaunchArgument(
        'flir_frame_id', default_value='boson_camera',
        description='The TF frame ID stamped on each Image header'
    )
    dev_arg = DeclareLaunchArgument(
        'flir_dev', default_value=get_flir_port(),
        description='The Linux video device path for the camera'
    )
    frame_rate_arg = DeclareLaunchArgument(
        'flir_frame_rate', default_value='30.0',
        description='V4L2 polling rate in Hz.'
    )
    video_mode_arg = DeclareLaunchArgument(
        'flir_video_mode', default_value='YUV',
        description='YUV, RAW16, or RAW16_AGC'
    )
    publish_color_arg = DeclareLaunchArgument(
        'flir_publish_color', default_value='False',
        description='Publish 3-channel BGR colorized output instead of mono8'
    )
    raw16_agc_low_pct_arg = DeclareLaunchArgument(
        'flir_raw16_agc_low_pct', default_value='1.0',
    )
    raw16_agc_high_pct_arg = DeclareLaunchArgument(
        'flir_raw16_agc_high_pct', default_value='1.0',
    )
    camera_info_url_arg = DeclareLaunchArgument(
        'flir_camera_info_url', default_value='',
    )

    boson_camera_node = Node(
        package='flir_boson_usb2',
        executable='boson_camera_node',
        name='flir_boson_usb_node',
        namespace=LaunchConfiguration('flir_namespace'), # Updated
        output='screen',
        parameters=[{
            'frame_id': LaunchConfiguration('flir_frame_id'),
            'dev': LaunchConfiguration('flir_dev'),                 # Updated
            'frame_rate': LaunchConfiguration('flir_frame_rate'),   # Updated
            'video_mode': LaunchConfiguration('flir_video_mode'),   # Updated
            'publish_color': LaunchConfiguration('flir_publish_color'),
            'raw16_agc_low_pct': LaunchConfiguration('flir_raw16_agc_low_pct'),
            'raw16_agc_high_pct': LaunchConfiguration('flir_raw16_agc_high_pct'),
            'camera_info_url': LaunchConfiguration('flir_camera_info_url'),
        }]
    )

    return LaunchDescription([
        namespace_arg,
        frame_id_arg,
        dev_arg,
        frame_rate_arg,
        video_mode_arg,
        publish_color_arg,
        raw16_agc_low_pct_arg,
        raw16_agc_high_pct_arg,
        camera_info_url_arg,
        boson_camera_node,
    ])