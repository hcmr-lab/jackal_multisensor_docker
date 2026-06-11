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
    """Launch the FLIR Boson USB camera driver.

    To load a camera calibration, pass camera_info_url on the command line.
    Common patterns:

      # A calibration shipped with this package (for quick testing only,
      # intrinsics are NOT guaranteed to match your physical camera):
      camera_info_url:=package://flir_boson_usb2/example_calibrations/Boson640.yaml

      # Your own calibration file on disk:
      camera_info_url:=file:///home/user/calibrations/my_boson.yaml

      # A calibration in another ROS package:
      camera_info_url:=package://my_robot_calibrations/boson_front.yaml

    Leave camera_info_url empty (the default) to publish uncalibrated
    CameraInfo. Downstream nodes that require calibration will need it
    set explicitly.
    """

    namespace_arg = DeclareLaunchArgument(
        'namespace', default_value='flir_boson',
        description='The ROS namespace for the camera node'
    )
    frame_id_arg = DeclareLaunchArgument(
        'frame_id', default_value='boson_camera',
        description='The TF frame ID stamped on each Image header'
    )
    
    # --- UPDATED: Now uses the auto-discovery function for the default value ---
    dev_arg = DeclareLaunchArgument(
        'dev', default_value=get_flir_port(),
        description='The Linux video device path for the camera'
    )
    # -------------------------------------------------------------------------
    
    frame_rate_arg = DeclareLaunchArgument(
        'frame_rate', default_value='30.0',
        description='V4L2 polling rate in Hz. Any positive value is accepted; '
                    'invalid values are clamped to 1.0 Hz. Typical Boson '
                    'hardware rates are 9, 30, or 60.'
    )
    video_mode_arg = DeclareLaunchArgument(
        'video_mode', default_value='YUV',
        description='YUV: hardware-AGC mono8/bgr8 from camera DSP (low CPU). '
                    'RAW16: raw 16-bit thermal counts, published as mono16. '
                    'RAW16_AGC: host-side percentile AGC, published as mono8.'
    )
    publish_color_arg = DeclareLaunchArgument(
        'publish_color', default_value='False',
        description='Publish 3-channel BGR colorized output instead of mono8 '
                    '(YUV mode only)'
    )
    raw16_agc_low_pct_arg = DeclareLaunchArgument(
        'raw16_agc_low_pct', default_value='1.0',
        description='Bottom-tail clip percentage for RAW16_AGC (e.g. 1.0 '
                    'discards the darkest 1% of pixels). Valid range: [0, 50).'
    )
    raw16_agc_high_pct_arg = DeclareLaunchArgument(
        'raw16_agc_high_pct', default_value='1.0',
        description='Top-tail clip percentage for RAW16_AGC (e.g. 1.0 '
                    'discards the brightest 1% of pixels). Valid range: [0, 50).'
    )
    camera_info_url_arg = DeclareLaunchArgument(
        'camera_info_url', default_value='',
        description='Camera calibration file URL (file:// or package://). '
                    'Empty (default) publishes uncalibrated CameraInfo. See '
                    'header docstring for examples.'
    )

    boson_camera_node = Node(
        package='flir_boson_usb2',
        executable='boson_camera_node',
        name='flir_boson_usb_node',
        namespace=LaunchConfiguration('namespace'),
        output='screen',
        parameters=[{
            'frame_id': LaunchConfiguration('frame_id'),
            'dev': LaunchConfiguration('dev'),
            'frame_rate': LaunchConfiguration('frame_rate'),
            'video_mode': LaunchConfiguration('video_mode'),
            'publish_color': LaunchConfiguration('publish_color'),
            'raw16_agc_low_pct': LaunchConfiguration('raw16_agc_low_pct'),
            'raw16_agc_high_pct': LaunchConfiguration('raw16_agc_high_pct'),
            'camera_info_url': LaunchConfiguration('camera_info_url'),
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