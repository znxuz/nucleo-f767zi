#include "wheel_ctrl.hpp"

#include <rclc/executor.h>
#include <rclc/subscription.h>

#include <nucleo_144/micro_ros/WheelDataWrapper.hpp>

#include "logger.hpp"

extern logger logger;

extern "C" {
static rclc_executor_t wheel_ctrl_exe;
static WheelDataWrapper msg_buffer{};
static rcl_subscription_t sub_wheel_vel;

static void wheel_vel_callback(const void* arg) {
  const auto* msg = reinterpret_cast<const decltype(msg_buffer.msg)*>(arg);
  logger.log("wheel_vel_cb wheel velocities: [%.2f], [%.2f], [%.2f], [%.2f]",
             msg->data.data[0], msg->data.data[1], msg->data.data[2],
             msg->data.data[3]);

  /* same outputs for both */
  // logger.log("casted msg data ptr: %lu",
  // reinterpret_cast<uintptr_t>(msg->data.data)); logger.log("msg_buffer data
  // ptr: %lu", reinterpret_cast<uintptr_t>(msg_buffer.msg.data.data));
}

rclc_executor_t* wheel_ctrl_init(rcl_node_t* node, rclc_support_t* support,
                                 const rcl_allocator_t* allocator) {
  wheel_ctrl_exe = rclc_executor_get_zero_initialized_executor();
  rclc_executor_init(&wheel_ctrl_exe, &support->context, 1, allocator);

  rclc_subscription_init_default(
      &sub_wheel_vel, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
      "wheel_vel");

  rclc_executor_add_subscription(&wheel_ctrl_exe, &sub_wheel_vel,
                                 &msg_buffer.msg, wheel_vel_callback,
                                 ON_NEW_DATA);

  return &wheel_ctrl_exe;
}
}
