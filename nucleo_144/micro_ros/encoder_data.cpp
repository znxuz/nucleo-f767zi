#include "encoder_data.hpp"

#include <rcl/time.h>
#include <rclc/executor.h>
#include <rclc/publisher.h>
#include <rclc/timer.h>
#include <rmw_microros/time_sync.h>

#include <array>
#include <nucleo_144/micro_ros/rcl_ret_check.hpp>

#include "WheelDataWrapper.hpp"
#include "nucleo_144/micro_ros/logger.hpp"

static constexpr uint8_t N_EXEC_HANDLES = 1;
static constexpr uint16_t TIMER_TIMEOUT_MS = 2000;

extern "C" {
static auto encoder_pub_exe = rclc_executor_get_zero_initialized_executor();
static auto timer = rcl_get_zero_initialized_timer();
static rcl_publisher_t pub_encoder;

extern logger logger;

static void encoder_data_cb(rcl_timer_t* timer, int64_t time_diff) {
  WheelDataWrapper<double, WheelDataType::ENC_DELTA_RAD> enc_data{
      std::array<double, 4>{1, 2, 3, 4}};

  logger.log("[en]: pub enc");
  rcl_ret_check(rcl_publish(&pub_encoder, &enc_data.msg, NULL));
}

rclc_executor_t* encoder_data_exe_init(const rcl_node_t* node,
                                       rclc_support_t* support,
                                       const rcl_allocator_t* allocator) {
  rclc_executor_init(&encoder_pub_exe, &support->context, N_EXEC_HANDLES,
                     allocator);

  rclc_timer_init_default2(&timer, support, RCL_MS_TO_NS(TIMER_TIMEOUT_MS),
                           &encoder_data_cb, true);
  rclc_executor_add_timer(&encoder_pub_exe, &timer);

  rcl_ret_check(rclc_publisher_init_default(
      &pub_encoder, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
      "encoder_data"));

  return &encoder_pub_exe;
}
}
