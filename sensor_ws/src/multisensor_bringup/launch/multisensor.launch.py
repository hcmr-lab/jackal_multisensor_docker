import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction, LogInfo, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource, XMLLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    
    # 1. Options (Launch arguments)
    args = [
        DeclareLaunchArgument('launch_realsense', default_value='true', description='Launch RealSense camera'),
        DeclareLaunchArgument('launch_ximea', default_value='true', description='Launch Ximea camera'),
        DeclareLaunchArgument('launch_ouster', default_value='true', description='Launch Ouster LiDAR'),
        DeclareLaunchArgument('launch_flir', default_value='true', description='Launch FLIR Boson thermal camera'),
        DeclareLaunchArgument('launch_xsens', default_value='true', description='Launch Xsens IMU'),
        
        # FLIR dynamic overrides
        DeclareLaunchArgument('flir_dev', default_value='/dev/video0', description='FLIR video port'),
    ]

    launch_realsense = LaunchConfiguration('launch_realsense')
    launch_ximea = LaunchConfiguration('launch_ximea')
    launch_ouster = LaunchConfiguration('launch_ouster')
    launch_flir = LaunchConfiguration('launch_flir')
    launch_xsens = LaunchConfiguration('launch_xsens')

    # Get our own bringup package directory
    my_bringup_dir = get_package_share_directory('multisensor_bringup')

    # 2. Define the launches
    
    # Xsens IMU (Custom Wrapper)
    xsens_launch = IncludeLaunchDescription(
        XMLLaunchDescriptionSource(os.path.join(my_bringup_dir, 'launch', 'xsens.launch.xml')),
        launch_arguments={
            'initial_wait': '1.0',
            #'device': '/dev/ttyUSB0',
            #'baudrate': '115200',
        }.items(),
        condition=IfCondition(launch_xsens)
    )

    # Ouster LiDAR (Direct Call)
    ouster_dir = get_package_share_directory('ouster_ros')
    ouster_launch = IncludeLaunchDescription(
        XMLLaunchDescriptionSource(os.path.join(ouster_dir, 'launch', 'sensor.launch.xml')),
        launch_arguments={
            'sensor_hostname': 'os-122212000760.local',
            'timestamp_mode': 'TIME_FROM_ROS_TIME',
        }.items(),
        condition=IfCondition(launch_ouster)
    )

    # RealSense Camera (Direct Call, Delayed via TimerAction)
    realsense_dir = get_package_share_directory('realsense2_camera')
    realsense_include = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(realsense_dir, 'launch', 'rs_launch.py')),
        launch_arguments={
                'initial_reset': 'true',
                
                # Disable streams you don't need (Saves massive USB bandwidth)
                'enable_depth': 'false',
                'enable_infra': 'false',
                'enable_infra1': 'false',
                'enable_infra2': 'false',
                'enable_gyro': 'false',
                'enable_accel': 'false',
                
                # 3. Explicitly configure your RGB stream
                'enable_color': 'true',
                'rgb_camera.color_profile': '640x480x30',
                'rgb_camera.color_format': 'YUYV',
        }.items()
    )
    realsense_launch = TimerAction(
        period=3.0,
        actions=[LogInfo(msg="Starting RealSense..."), realsense_include],
        condition=IfCondition(launch_realsense)
    )

    # Ximea Camera (Custom Wrapper, 5s Delay)
    ximea_include = IncludeLaunchDescription(
        XMLLaunchDescriptionSource(os.path.join(my_bringup_dir, 'launch', 'ximea.launch.xml'))
    )
    ximea_cameras_launch = TimerAction(
        period=5.0,
        actions=[LogInfo(msg="Starting Ximea..."), ximea_include],
        condition=IfCondition(launch_ximea)
    )

    # FLIR inclusion (Direct Call)
    flir_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(my_bringup_dir, 'launch', 'flir.launch.py')),
        launch_arguments={
            'video_mode': 'YUV',
            'frame_rate': '30.0',
        }.items(),
        condition=IfCondition(launch_flir)
    )

    # 3. Assemble
    return LaunchDescription(
        args + [
            LogInfo(msg="Starting Multisensor Bringup..."),
            xsens_launch,
            ouster_launch,
            realsense_launch,
            flir_launch,
            ximea_cameras_launch
        ]
    )