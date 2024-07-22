#include <cstdio>
#include <rcl/allocator.h>
#include <rcl/error_handling.h>
#include <rcl/node.h>
#include <rcl/publisher.h>
#include <rcl/rcl.h>
#include <rcl/subscription.h>
#include <rclc/executor.h>
#include <rclc/executor_handle.h>
#include <rclc/publisher.h>
#include <rclc/rclc.h>
#include <rclc/subscription.h>
#include <rclc/types.h>
#include <rcutils/logging.h>
#include <rmw_microros/rmw_microros.h>
#include <rmw_microxrcedds_c/config.h>
#include <uxr/client/transport.h>

#include <geometry_msgs/msg/twist.h>
#include <std_msgs/msg/int8.h>
#include <std_msgs/msg/float64_multi_array.h>

#include <cmsis_os2.h>
#include <stm32f7xx_hal.h>

#include <experimental/source_location>

#include "logger.hpp"
#include "wheel_vel_msg.h"

static inline void
rcl_ret_check(rcl_ret_t ret_code,
			  const std::experimental::source_location location
			  = std::experimental::source_location::current())
{
	if (ret_code) {
		printf("Failed status on line %d: %d in %s",
			   static_cast<int>(location.line()), static_cast<int>(ret_code),
			   location.file_name());
	}
}

extern "C"
{

bool cubemx_transport_open(struct uxrCustomTransport* transport);
bool cubemx_transport_close(struct uxrCustomTransport* transport);
size_t cubemx_transport_write(struct uxrCustomTransport* transport,
							  const uint8_t* buf, size_t len, uint8_t* err);
size_t cubemx_transport_read(struct uxrCustomTransport* transport, uint8_t* buf,
							 size_t len, int timeout, uint8_t* err);

void* microros_allocate(size_t size, void* state);
void microros_deallocate(void* pointer, void* state);
void* microros_reallocate(void* pointer, size_t size, void* state);
void* microros_zero_allocate(size_t number_of_elements, size_t size_of_element,
							 void* state);

rcl_allocator_t allocator;
rclc_support_t support;

void init(void* arg)
{
	UART_HandleTypeDef* huart3 = (UART_HandleTypeDef*)arg;
	rmw_uros_set_custom_transport(
		true, (void*)huart3, cubemx_transport_open, cubemx_transport_close,
		cubemx_transport_write, cubemx_transport_read);

	rcl_allocator_t freeRTOS_allocator
		= rcutils_get_zero_initialized_allocator();
	freeRTOS_allocator.allocate = microros_allocate;
	freeRTOS_allocator.deallocate = microros_deallocate;
	freeRTOS_allocator.reallocate = microros_reallocate;
	freeRTOS_allocator.zero_allocate = microros_zero_allocate;

	if (!rcutils_set_default_allocator(&freeRTOS_allocator))
		printf("Error on default allocators (line %d)\n", __LINE__);

	allocator = rcl_get_default_allocator();
	rclc_support_init(&support, 0, NULL, &allocator);
}

rcl_publisher_t pub_wheel_vel;
void pose_callback(const void* arg)
{
	const auto* pose_msg = reinterpret_cast<const geometry_msgs__msg__Twist*>(arg);
	logger.log("pose callback pose: [x: %.2f, y: %.2f, theta: %.2f]",
			   pose_msg->linear.x, pose_msg->linear.y, pose_msg->angular.z);

	wheel_vel_msg wheel_msg;
	wheel_msg[0] = 1.3;
	wheel_msg[1] = 2.5;
	wheel_msg[2] = 3.7;
	wheel_msg[3] = 7.3;

	rcl_ret_check(rcl_publish(&pub_wheel_vel, &wheel_msg.msg, NULL));
}

rcl_publisher_t pub_odometry;
void odometry_callback(rcl_timer_t* timer, int64_t last_call_time)
{
	geometry_msgs__msg__Twist pose_msg{};
	pose_msg.linear.x = 1;
	pose_msg.linear.y = 2;
	pose_msg.angular.z = 0.5;
	rcl_ret_check(rcl_publish(&pub_odometry, &pose_msg, NULL));
}

void wheel_vel_callback(const void* arg)
{
	const auto* wheel_vel_msg
		= reinterpret_cast<const std_msgs__msg__Float64MultiArray*>(arg);
	logger.log(
		"wheel_vel_callback wheel velocities: [%.2f], [%.2f], [%.2f], [%.2f]",
		wheel_vel_msg->data.data[0], wheel_vel_msg->data.data[1],
		wheel_vel_msg->data.data[2], wheel_vel_msg->data.data[3]);
}

void micro_ros(void* arg)
{
	init(arg);

	rcl_node_t node;
	rclc_node_init_default(&node, "micro_ros_node", "", &support);

	logger.init(&node);

	auto odometry_exe = rclc_executor_get_zero_initialized_executor();
	rclc_executor_init(&odometry_exe, &support.context, 1, &allocator);

	rclc_publisher_init_default(
		&pub_odometry, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "odometry");
	auto timer = rcl_get_zero_initialized_timer();
	const unsigned int timer_timeout = 2000;
	rclc_timer_init_default2(&timer, &support, RCL_MS_TO_NS(timer_timeout),
							 odometry_callback, true);
	rclc_executor_add_timer(&odometry_exe, &timer);

	auto interpolate_exe = rclc_executor_get_zero_initialized_executor();
	rclc_executor_init(&interpolate_exe, &support.context, 2, &allocator);

	rcl_subscription_t sub_odometry;
	rclc_subscription_init_default(
		&sub_odometry, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "odometry");
	auto odometry_msg = geometry_msgs__msg__Twist();
	rclc_executor_add_subscription(&interpolate_exe, &sub_odometry,
								   &odometry_msg, &pose_callback, ON_NEW_DATA);

	rcl_subscription_t sub_cmd_vel;
	rclc_subscription_init_default(
		&sub_cmd_vel, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "cmd_vel");
	auto cmd_vel_msg = geometry_msgs__msg__Twist();
	rclc_executor_add_subscription(&interpolate_exe, &sub_cmd_vel, &cmd_vel_msg,
								   &pose_callback, ON_NEW_DATA);

	rclc_publisher_init_default(
		&pub_wheel_vel, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray), "wheel_vel");

	rcl_subscription_t sub_wheel_vel;
	rclc_subscription_init_default(
		&sub_wheel_vel, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
		"wheel_vel");
	auto action_exe = rclc_executor_get_zero_initialized_executor();
	rclc_executor_init(&action_exe, &support.context, 1, &allocator);

	wheel_vel_msg msg;
	rclc_executor_add_subscription(&action_exe, &sub_wheel_vel,
			&msg.msg, &wheel_vel_callback, ON_NEW_DATA);

	logger.log("debug: starting the loop");
	for (;;) {
		rclc_executor_spin_some(&odometry_exe, RCL_MS_TO_NS(1));
		rclc_executor_spin_some(&interpolate_exe, RCL_MS_TO_NS(1));
		rclc_executor_spin_some(&action_exe, RCL_MS_TO_NS(1));
	}
}
}
