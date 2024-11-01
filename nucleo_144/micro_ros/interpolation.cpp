#include "interpolation.hpp"

#include <geometry_msgs/msg/pose2_d.h>
#include <geometry_msgs/msg/twist.h>
#include <rcl/allocator.h>
#include <rcl/node.h>
#include <rcl/time.h>
#include <rclc/executor.h>
#include <rclc/executor_handle.h>
#include <rclc/publisher.h>
#include <rclc/subscription.h>
#include <rclc/timer.h>
#include <rclc/types.h>

#include <nucleo_144/mrcpptypes.hpp>

#include "WheelDataWrapper.hpp"
#include "nucleo_144/micro_ros/logger.hpp"
#include "rcl_ret_check.hpp"

static constexpr uint8_t N_EXEC_HANDLES = 3;
static constexpr double UROS_FREQ_MOD_INTERPOLATION_SEC = 5;
static constexpr uint16_t TIMER_TIMEOUT_MS =
    UROS_FREQ_MOD_INTERPOLATION_SEC * 1000;

extern "C" {
static rclc_executor_t interpolation_exe;

static rcl_subscription_t sub_cmd_vel;
static geometry_msgs__msg__Twist msg_cmd_vel{};

static rcl_subscription_t sub_odometry;
static geometry_msgs__msg__Pose2D msg_odom_pose{};

static auto interpolation_timer = rcl_get_zero_initialized_timer();
static rcl_publisher_t pub_wheel_vel;

static auto vel_rf_target = vPose<double>{};
static auto pose_cur = Pose<double>{};

extern logger logger;

static void cmd_vel_cb(const void*) {
  vel_rf_target.vx = msg_cmd_vel.linear.x * 1000.0;  // m/s -> mm/s
  vel_rf_target.vy = msg_cmd_vel.linear.y * 1000.0;  // m/s -> mm/s
  vel_rf_target.omega = msg_cmd_vel.angular.z;       // rad/s
}

static void pose_cb(const void*) {
  pose_cur = {msg_odom_pose.x, msg_odom_pose.y, msg_odom_pose.theta};
}

static void interpolation_cb(rcl_timer_t*, int64_t) {
  logger.log("[interpolation] cb");

  auto msg_vel_wheel_sp = WheelDataWrapper<double, WheelDataType::VEL_SP>{};
  rcl_ret_check(rcl_publish(&pub_wheel_vel, &msg_vel_wheel_sp.msg, NULL));
}

rclc_executor_t* interpolation_init(rcl_node_t* node, rclc_support_t* support,
                                    const rcl_allocator_t* allocator) {
  rcl_ret_check(rclc_executor_init(&interpolation_exe, &support->context,
                                   N_EXEC_HANDLES, allocator));

  rcl_ret_check(rclc_subscription_init_default(
      &sub_cmd_vel, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "cmd_vel"));
  rcl_ret_check(rclc_executor_add_subscription(&interpolation_exe, &sub_cmd_vel,
                                               &msg_cmd_vel, &cmd_vel_cb,
                                               ON_NEW_DATA));

  rcl_ret_check(rclc_subscription_init_default(
      &sub_odometry, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Pose2D), "odometry"));
  rcl_ret_check(rclc_executor_add_subscription(&interpolation_exe,
                                               &sub_odometry, &msg_odom_pose,
                                               &pose_cb, ON_NEW_DATA));

  rcl_ret_check(rclc_timer_init_default2(&interpolation_timer, support,
                                         RCL_MS_TO_NS(TIMER_TIMEOUT_MS),
                                         &interpolation_cb, true));
  rcl_ret_check(
      rclc_executor_add_timer(&interpolation_exe, &interpolation_timer));

  rcl_ret_check(rclc_publisher_init_default(
      &pub_wheel_vel, node,
      WheelDataWrapper<double, WheelDataType::VEL_SP>::get_msg_type_support(),
      "wheel_vel"));

  return &interpolation_exe;
}
}
