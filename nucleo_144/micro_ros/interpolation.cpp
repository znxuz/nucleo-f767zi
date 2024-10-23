#include "interpolation.hpp"

#include <geometry_msgs/msg/twist.h>
#include <rcl/allocator.h>
#include <rcl/node.h>
#include <rclc/executor.h>
#include <rclc/executor_handle.h>
#include <rclc/subscription.h>

#include <array>

#include "WheelDataWrapper.hpp"
#include "logger.hpp"
#include "rcl_ret_check.hpp"

extern logger logger;

extern "C" {
static rclc_executor_t interpolation_exe =
    rclc_executor_get_zero_initialized_executor();

// static rcl_subscription_t sub_odometry;
// static geometry_msgs__msg__Twist odometry_msg{};

static rcl_subscription_t sub_cmd_vel;
static geometry_msgs__msg__Twist cmd_vel_msg{};
static rcl_publisher_t pub_wheel_vel;

static void interpolation_cb(const void* arg) {
  WheelDataWrapper<double, WheelDataType::VEL_SP> wheel_sp_msg{
      std::array{1.3, 2.5, 3.7, 7.3}};
  static double counter = 1.2;
  wheel_sp_msg[0] += ++counter;

  rcl_ret_check(rcl_publish(&pub_wheel_vel, &wheel_sp_msg.msg, NULL));
}

rclc_executor_t* interpolation_init(rcl_node_t* node, rclc_support_t* support,
                                    const rcl_allocator_t* allocator) {
  rcl_ret_check(
      rclc_executor_init(&interpolation_exe, &support->context, 1, allocator));

  // rclc_subscription_init_default(
  //     &sub_odometry, node,
  //     ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "odometry");
  // rclc_executor_add_subscription(&interpolation_exe, &sub_odometry,
  //                                &odometry_msg, pose_callback, ON_NEW_DATA);

  rcl_ret_check(rclc_subscription_init_default(
      &sub_cmd_vel, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "cmd_vel"));
  rcl_ret_check(rclc_executor_add_subscription(&interpolation_exe, &sub_cmd_vel,
                                               &cmd_vel_msg, &interpolation_cb,
                                               ON_NEW_DATA));

  rcl_ret_check(rclc_publisher_init_default(
      &pub_wheel_vel, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
      "wheel_vel"));

  return &interpolation_exe;
}
}
