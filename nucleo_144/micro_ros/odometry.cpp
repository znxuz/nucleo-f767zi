#include "odometry.hpp"

#include <geometry_msgs/msg/pose2_d.h>
#include <rclc/publisher.h>
#include <rclc/subscription.h>
#include <rclc/types.h>

#include <nucleo_144/mrcpptypes.hpp>
#include "WheelDataWrapper.hpp"
#include "nucleo_144/micro_ros/logger.hpp"
#include "rcl_ret_check.hpp"

static constexpr uint8_t N_EXEC_HANDLES = 1;

extern "C" {
static auto odometry_exe = rclc_executor_get_zero_initialized_executor();

static rcl_subscription_t sub_encoder_data;
static auto msg_enc_data =
    WheelDataWrapper<double, WheelDataType::ENC_DELTA_RAD>{};
static Pose<double> pose_wf;

static rcl_publisher_t pub_odometry;

extern logger logger;

static void odometry_cb(const void* arg) {
  /*
   * odometry: encoder delta gets feeded directly into the inverted jacobian
   * matrix without dividing the dt for the reason being:
   * d_enc in rad * r / dt = vel -> tf() = robot_vel * dt = dpose
   * => dt can be spared because its unnecessary calculation
   */
  // VelRF dpose_rf_mtx =
  //     vWheel2vRF(VelWheel(enc_delta->data.data) * robot_params.wheel_radius);
  // Pose<double> dpose_rf{dpose_rf_mtx(0), dpose_rf_mtx(1), dpose_rf_mtx(2)};
  // // update_epsilon(dpose_rframe_matrix(3)); // factor for this is zero anyway
  //
  // // ASK: why divide the sum by 2
  // Pose<double> dpose_wf = pRF2pWF(
  //     dpose_rf, pose_wf.theta + dpose_rf.theta / static_cast<double>(2));
  // pose_wf += dpose_wf;
  //
  logger.log("[odometry]: pose cb");

  auto msg = geometry_msgs__msg__Pose2D{pose_wf.x, pose_wf.y, pose_wf.theta};
  rcl_ret_check(rcl_publish(&pub_odometry, &msg, NULL));
}

rclc_executor_t* odometry_init(rcl_node_t* node, rclc_support_t* support,
                               const rcl_allocator_t* allocator) {
  rcl_ret_check(rclc_executor_init(&odometry_exe, &support->context,
                                   N_EXEC_HANDLES, allocator));

  rclc_subscription_init_default(
      &sub_encoder_data, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
      "encoder_data");
  rcl_ret_check(rclc_executor_add_subscription(&odometry_exe, &sub_encoder_data,
                                               &msg_enc_data.msg, &odometry_cb,
                                               ON_NEW_DATA));

  rcl_ret_check(rclc_publisher_init_default(
      &pub_odometry, node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Pose2D), "odometry"));

  return &odometry_exe;
}
}
