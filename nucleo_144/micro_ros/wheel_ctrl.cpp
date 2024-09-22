#include "wheel_ctrl.hpp"
#include "nucleo_144/micro_ros/wheel_vel_msg.h"
#include "rclc/executor.h"
#include "rclc/subscription.h"
#include "logger.hpp"

extern logger logger;

extern "C"
{
static rclc_executor_t wheel_ctrl_exe;
static wheel_vel_msg msg{};
static rcl_subscription_t sub_wheel_vel;

static void wheel_vel_callback(const void* arg)
{
	const auto* wheel_vel_msg
		= reinterpret_cast<const struct wheel_vel_msg*>(arg);
	logger.log(
		"wheel_vel_cb wheel velocities: [%.2f], [%.2f], [%.2f], [%.2f]",
		wheel_vel_msg->operator[](0), wheel_vel_msg->operator[](1),
		wheel_vel_msg->operator[](2), wheel_vel_msg->operator[](3));
}

rclc_executor_t* wheel_ctrl_init(rcl_node_t* node,
						rclc_support_t* support,
						const rcl_allocator_t* allocator)
{
	wheel_ctrl_exe = rclc_executor_get_zero_initialized_executor();
	rclc_executor_init(&wheel_ctrl_exe, &support->context, 1, allocator);

	rclc_subscription_init_default(
			&sub_wheel_vel, node,
			ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
			"wheel_vel");

	rclc_executor_add_subscription(&wheel_ctrl_exe, &sub_wheel_vel, &msg.msg,
			&wheel_vel_callback, ON_NEW_DATA);

	return &wheel_ctrl_exe;
}
}
