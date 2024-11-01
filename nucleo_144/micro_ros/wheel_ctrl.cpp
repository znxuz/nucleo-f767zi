#include "wheel_ctrl.hpp"

#include <rclc/executor.h>
#include <rclc/subscription.h>

#include <algorithm>

#include "WheelDataWrapper.hpp"
#include "nucleo_144/micro_ros/logger.hpp"
#include "rcl_ret_check.hpp"
#include "rclc/executor_handle.h"

static constexpr uint8_t N_EXEC_HANDLES = 2;

extern "C" {
static auto wheel_ctrl_exe = rclc_executor_get_zero_initialized_executor();

static rcl_subscription_t sub_encoder_data;
static auto enc_msg_buf =
    WheelDataWrapper<double, WheelDataType::ENC_DELTA_RAD>{};

static rcl_subscription_t sub_wheel_vel;
static auto wheel_vel_buf = WheelDataWrapper<double, WheelDataType::VEL_SP>{};

extern logger logger;

static void enc2vel_cb(const void* arg) {
  // const auto* msg = reinterpret_cast<const MsgType<double>*>(arg);
  // ULOG_DEBUG("[wheel ctrl - enc_cb]: enc: [%.2f, %.2f, %.2f, %.2f]",
  //            msg->data.data[0], msg->data.data[1], msg->data.data[2],
  //            msg->data.data[3]);
  //
  // std::transform(
  //     msg->data.data, msg->data.data + N_WHEEL, begin(wheel_vel_cur),
  //     [dt = UROS_FREQ_MOD_ENC_SEC, r = robot_params.wheel_radius](
  //         double encoder_delta_rad) { return encoder_delta_rad * r / dt; });
  // ULOG_DEBUG(
  //     "[wheel ctrl - enc_cb]: wheel vel from enc: [%.2f, %.2f, %.2f, %.2f]",
  //     wheel_vel_cur[0], wheel_vel_cur[1], wheel_vel_cur[2], wheel_vel_cur[3]);

  logger.log("[wheel ctrl]: enc cb");
}


static void wheel_ctrl_cb(const void* arg) {
  // if (arg)
  //   std::copy(wheel_vel_buf.msg.data.data,
  //             wheel_vel_buf.msg.data.data + N_WHEEL, begin(wheel_vel_sp));

  // // TODO: refactor away the conversion from array to mtx
  // VelWheel vel_cur = VelWheel(wheel_vel_cur.data());
  // VelWheel vel_sp = VelWheel(wheel_vel_sp.data());
  //
  // ULOG_DEBUG("%s: %.2f, %.2f, %.2f, %.2f",
  //            "[wheel ctrl - wheel_ctrl_cb]: vel_wheel_sp from interpolation",
  //            vel_sp(0), vel_sp(1), vel_sp(2), vel_sp(3));
  //
  // auto vel_corrected = vel_pid(vel_cur, vel_sp);
  //
  // ULOG_DEBUG("%s: %.2f, %.2f, %.2f, %.2f",
  //            "[wheel ctrl - wheel_ctrl_cb]: vel corrected", vel_corrected(0),
  //            vel_corrected(1), vel_corrected(2), vel_corrected(3));
  //
  // hal_wheel_vel_set_pwm(vel_to_duty_cycle(vel_corrected));

  logger.log("[wheel ctrl]: wheel ctrl cb");
}

rclc_executor_t* wheel_ctrl_init(rcl_node_t* node, rclc_support_t* support,
                                 const rcl_allocator_t* allocator) {
  rcl_ret_check(rclc_executor_init(&wheel_ctrl_exe, &support->context,
                                   N_EXEC_HANDLES, allocator));

  rcl_ret_check(rclc_subscription_init_default(
      &sub_encoder_data, node,
      WheelDataWrapper<double,
                       WheelDataType::ENC_DELTA_RAD>::get_msg_type_support(),
      "encoder_data"));
  rcl_ret_check(rclc_executor_add_subscription(
      &wheel_ctrl_exe, &sub_encoder_data, &enc_msg_buf.msg, &enc2vel_cb,
      ON_NEW_DATA));

  rcl_ret_check(rclc_subscription_init_default(
      &sub_wheel_vel, node,
      WheelDataWrapper<double, WheelDataType::VEL_SP>::get_msg_type_support(),
      "wheel_vel"));
  /*
   * ALWAYS: let wheel ctrl callback execute at the same frequency as the
   * encoder data publisher, and also execute when a new wheel vel is available
   */
  rcl_ret_check(rclc_executor_add_subscription(&wheel_ctrl_exe, &sub_wheel_vel,
                                               &wheel_vel_buf.msg,
                                               &wheel_ctrl_cb, ALWAYS));

  rcl_ret_check(rclc_executor_set_trigger(&wheel_ctrl_exe,
                                          &rclc_executor_trigger_any, NULL));

  return &wheel_ctrl_exe;
}
}
