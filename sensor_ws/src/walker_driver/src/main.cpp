#include <chrono>
#include <memory>
#include <mutex>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "sensor_msgs/msg/imu.hpp"

#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include "walker_driver/HDC2450Controller.h"
#include "walker_driver/Velocity.h"
#include "walker_driver/Robot.h"

using namespace std::chrono_literals;

class WalkerDriverNode : public rclcpp::Node
{
public:
    // Update the constructor to accept the port as a parameter
    WalkerDriverNode(const std::string& usb_port) : Node("wheelchair_talker"), x_(0.0), y_(0.0), th_(0.0)
    {
        // Publishers and Subscribers
        odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 1000);
        cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel", 1000, std::bind(&WalkerDriverNode::velCallback, this, std::placeholders::_1));
        
        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

        // Controller Setup
        controller_.setBaudrate(115200);
        controller_.setCommandTimeout(35);
        controller_.setUSBport(usb_port); // <-- Now uses your command line argument!

        // Robot Configuration
        robot_.setB(0.475);
        robot_.setLimitMaxRPM(500);
        robot_.setLimitMinRPM(-500);
        robot_.setMaxRPM(100);
        robot_.setMinRPM(-100);
        robot_.setMtoW(1.0);
        robot_.setPulsesPerWheelRevolution(980);
        robot_.setR(0.1);
        robot_.setWtoM(1.0);
        robot_.setInvertLeft(false);
        robot_.setInvertRight(false);
        robot_.setSwapLeftRight(true);
        robot_.setSwapEncoders(true);

        if (controller_.open(500, true)) {
            RCLCPP_INFO(this->get_logger(), ":: Connected to HDC2450 Roboteq Controller at %s", usb_port.c_str());
        } else if (controller_.invalidInit()) {
            int il = 0;
            RCLCPP_WARN(this->get_logger(), ":: Detected a invalid output from the HDC2450, retrying...");
            while (il < 10) {
                RCLCPP_INFO(this->get_logger(), ":: Try %d of 10", il + 1);
                controller_.close();
                if (controller_.open(500, true)) {
                    RCLCPP_INFO(this->get_logger(), ":: Connected to HDC2450 Roboteq Controller at %s", usb_port.c_str());
                    break;
                }
                il++;
            }
            
            // If it still fails after 10 tries:
            if (il == 10) {
                RCLCPP_FATAL(this->get_logger(), ":: Failed to connect after retries.");
                rclcpp::shutdown();
                throw std::runtime_error("Hardware connection failed.");
            }

        } else {
            RCLCPP_FATAL(this->get_logger(), ":: Not Connected to HDC2450 Roboteq Controller at %s", usb_port.c_str());
            rclcpp::shutdown();
            throw std::runtime_error("Hardware connection failed."); // <-- This stops the RCLError crash
        }

        // 20Hz Timer (50ms)
        timer_ = this->create_wall_timer(50ms, std::bind(&WalkerDriverNode::timerCallback, this));
    }

    ~WalkerDriverNode()
    {
        controller_.issueCommand("!M 0 0\r");
        controller_.close();
    }

private:
    void velCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        Velocity VelocityCommand(msg->linear.x, msg->angular.z, robot_);

        std::lock_guard<std::mutex> lock(mtx_);
        bool status = controller_.setMotorCommand(VelocityCommand.leftVelocity, VelocityCommand.rightVelocity);

        if (!status) {
            RCLCPP_ERROR(this->get_logger(), ":: Error in Controller Request ( %d , %d )", VelocityCommand.rightVelocity, VelocityCommand.leftVelocity);
        }
    }

    void timerCallback()
    {
        auto current_time = this->now();

        std::vector<int> dataRe, dataAb, Voltage;

        mtx_.lock();
        dataRe = controller_.readEncoderCountRelative();
        Clock::staticDelay(5000000); 
        dataAb = controller_.readAbsoluteEncoderCount();
        Clock::staticDelay(5000000);
        Voltage = controller_.readInternalVoltages();
        mtx_.unlock();

        if(dataRe.size() < 2) return; // Failsafe

        double Left = (double)dataRe[1];
        double Right = -(double)dataRe[0];
        
        double dl = (Left / robot_.PulsesPerWheelRevolution) * 2.0 * M_PI * robot_.R;
        double dr = (Right / robot_.PulsesPerWheelRevolution) * 2.0 * M_PI * robot_.R;
        
        double displacement = (dr + dl) / 2.0;
        double angulardisplacement = (dr - dl) / robot_.b;
        
        double vx = displacement; 
        double vth = angulardisplacement;

        th_ += angulardisplacement;
        th_ = atan2(sin(th_), cos(th_));

        x_ += (displacement * cos(th_));
        y_ += (displacement * sin(th_));

        // TF Quaternion
        tf2::Quaternion q;
        q.setRPY(0, 0, th_);
        geometry_msgs::msg::Quaternion odom_quat = tf2::toMsg(q);

        // Transform Broadcaster
        geometry_msgs::msg::TransformStamped odom_trans;
        odom_trans.header.stamp = current_time;
        odom_trans.header.frame_id = "odom";
        odom_trans.child_frame_id = "base_link";
        odom_trans.transform.translation.x = x_;
        odom_trans.transform.translation.y = y_;
        odom_trans.transform.translation.z = 0.0;
        odom_trans.transform.rotation = odom_quat;
        tf_broadcaster_->sendTransform(odom_trans);

        // Odometry Message
        nav_msgs::msg::Odometry odom;
        odom.header.stamp = current_time;
        odom.header.frame_id = "odom";
        odom.child_frame_id = "base_link";

        odom.pose.pose.position.x = x_;
        odom.pose.pose.position.y = y_;
        odom.pose.pose.position.z = 0.0;
        odom.pose.pose.orientation = odom_quat;

        odom.twist.twist.linear.x = displacement * 20.0;
        odom.twist.twist.linear.y = 0.0;
        odom.twist.twist.linear.z = 0.0;
        odom.twist.twist.angular.z = angulardisplacement * 20.0;

        if (vx == 0.0 && vth == 0.0) {
            odom.pose.covariance[0] = 1e-9;
            odom.pose.covariance[7] = 1e-3;
            odom.pose.covariance[8] = 1e-9;
            odom.pose.covariance[14] = 1e6;
            odom.pose.covariance[21] = 1e6;
            odom.pose.covariance[28] = 1e6;
            odom.pose.covariance[35] = 1e-9;

            odom.twist.covariance[0] = 1e-9;
            odom.twist.covariance[7] = 1e-3;
            odom.twist.covariance[8] = 1e-9;
            odom.twist.covariance[14] = 1e6;
            odom.twist.covariance[21] = 1e6;
            odom.twist.covariance[28] = 1e6;
            odom.twist.covariance[35] = 1e-9;
        } else {
            odom.pose.covariance[0] = 1e-3;
            odom.pose.covariance[7] = 1e-3;
            odom.pose.covariance[14] = 1e6;
            odom.pose.covariance[21] = 1e6;
            odom.pose.covariance[28] = 1e6;
            odom.pose.covariance[35] = 1e3;

            odom.twist.covariance[0] = 1e-3;
            odom.twist.covariance[7] = 1e-3;
            odom.twist.covariance[14] = 1e6;
            odom.twist.covariance[21] = 1e6;
            odom.twist.covariance[28] = 1e6;
            odom.twist.covariance[35] = 1e3;
        }

        odom_pub_->publish(odom);
    }

    HDC2450Controller controller_;
    Robot robot_;
    std::mutex mtx_;

    double x_, y_, th_;

    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char **argv)
{
    // Check if the user provided a port, otherwise default to ACM0
    std::string port = "/dev/ttyACM0";
    if (argc >= 2) {
        port = argv[1];
    }

    rclcpp::init(argc, argv);
    auto node = std::make_shared<WalkerDriverNode>(port);
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
