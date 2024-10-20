#include "odometry.hpp"

#include <geometry_msgs/msg/twist.h>
#include <rcl/allocator.h>
#include <rcl/node.h>
#include <rclc/executor.h>
#include <rclc/executor_handle.h>
#include <rclc/timer.h>
#include <rclc/types.h>

#include <experimental/source_location>
#include <nucleo_144/micro_ros/logger.hpp>

#include "rcl_ret_check.hpp"

extern logger logger;

extern "C" {
static rclc_executor_t odometry_exe;
static inline const unsigned int TIMER_TIMEOUT = 2000;
static auto timer = rcl_get_zero_initialized_timer();
static geometry_msgs__msg__Twist pose_msg{};
static rcl_publisher_t pub_odometry;

static void odometry_callback(rcl_timer_t* timer, int64_t last_call_time) {
  pose_msg.linear.x = 1;
  pose_msg.linear.y = 2;
  pose_msg.angular.z = 0.5;
  rcl_ret_check(rcl_publish(&pub_odometry, &pose_msg, NULL));
  logger.log("odometry_cb published: [x: %.2f, y: %.2f, theta: %.2f]",
             pose_msg.linear.x, pose_msg.linear.y, pose_msg.angular.z);
}

rclc_executor_t* odometry_init(const rcl_node_t* node, rclc_support_t* support,
                               const rcl_allocator_t* allocator) {
  rclc_executor_init(&odometry_exe, &support->context, 1, allocator);

  rclc_publisher_init_default(
      &pub_odometry, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "odometry");

  rclc_timer_init_default2(&timer, support, RCL_MS_TO_NS(TIMER_TIMEOUT),
                           odometry_callback, true);
  rclc_executor_add_timer(&odometry_exe, &timer);

  return &odometry_exe;
}
}
