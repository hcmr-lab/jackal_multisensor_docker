cmake_minimum_required(VERSION 3.8)
project(walker_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -std=c++17)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Generate messages (This creates a target named 'walker_driver')
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobchairStream.msg"
  "msg/IMUStream.msg"
  DEPENDENCIES std_msgs geometry_msgs
)

# Add executable (Renamed to 'walker_driver_node' to avoid collision)
add_executable(walker_driver_node src/main.cpp)

# Specify include directories
target_include_directories(walker_driver_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Ament dependencies
ament_target_dependencies(walker_driver_node
  rclcpp
  geometry_msgs
  nav_msgs
  sensor_msgs
  std_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
)

# Get the typesupport target and link it to the executable (fixes the deprecation warning)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(walker_driver_node "${cpp_typesupport_target}")

# Install targets
install(TARGETS walker_driver_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

ament_package()
