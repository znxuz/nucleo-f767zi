#include "wheel_ctrl.hpp"

#include <rclc/executor.h>
#include <rclc/subscription.h>

#include <nucleo_144/micro_ros/WheelDataWrapper.hpp>

#include "WheelDataType.hpp"
#include "logger.hpp"
#include "rcl_ret_check.hpp"

extern logger logger;

extern "C" {
static rclc_executor_t wheel_ctrl_exe =
    rclc_executor_get_zero_initialized_executor();

static rcl_subscription_t sub_encoder_data;
static WheelDataWrapper<double, WheelDataType::ENC_DELTA_RAD> enc_msg_buf{};

static rcl_subscription_t sub_wheel_vel;
static WheelDataWrapper<double, WheelDataType::VEL_SP> wheel_vel_buf{};

// const void* arg not used because its the pointer for msg_buffer

static void wheel_ctrl_cb(const void* arg) {
  const auto* msg = reinterpret_cast<const MsgType<double>*>(arg);
  const auto* label = extract_label(*msg);

  switch (parse_wheel_data_type(label).value()) {
    case WheelDataType::ENC_DELTA_RAD:
      logger.log("%s: %.2f, %.2f, %.2f, %.2f", "d_enc: ", enc_msg_buf[0],
                 enc_msg_buf[1], enc_msg_buf[2], enc_msg_buf[3]);
      break;
    case WheelDataType::VEL_SP:
      logger.log("%s: %.2f, %.2f, %.2f, %.2f", "vel sp: ", wheel_vel_buf[0],
                 wheel_vel_buf[1], wheel_vel_buf[2], wheel_vel_buf[3]);
      break;
  }
}

rclc_executor_t* wheel_ctrl_init(rcl_node_t* node, rclc_support_t* support,
                                 const rcl_allocator_t* allocator) {
  rclc_executor_init(&wheel_ctrl_exe, &support->context, 2, allocator);

  rclc_subscription_init_default(
      &sub_encoder_data, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
      "encoder_data");
  rcl_ret_check(rclc_executor_add_subscription(
      &wheel_ctrl_exe, &sub_encoder_data, &enc_msg_buf.msg, &wheel_ctrl_cb,
      ON_NEW_DATA));

  rcl_ret_check(rclc_subscription_init_default(
      &sub_wheel_vel, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
      "wheel_vel"));
  rcl_ret_check(rclc_executor_add_subscription(&wheel_ctrl_exe, &sub_wheel_vel,
                                               &wheel_vel_buf.msg,
                                               &wheel_ctrl_cb, ON_NEW_DATA));

  rcl_ret_check(rclc_executor_set_trigger(&wheel_ctrl_exe,
                                          rclc_executor_trigger_any, NULL));

  return &wheel_ctrl_exe;
}
}
