#include <cstdio>
#include <experimental/source_location>

#include <cmsis_os2.h>
#include <stm32f7xx_hal.h>

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
#include <rclc/types.h>
#include <rcutils/logging.h>
#include <rmw_microros/rmw_microros.h>
#include <rmw_microxrcedds_c/config.h>
#include <uxr/client/transport.h>

#include <geometry_msgs/msg/twist.h>
#include <std_msgs/msg/float64_multi_array.h>
#include <std_msgs/msg/int8.h>

#include <nucleo_144/micro_ros/interpolation.hpp>
#include <nucleo_144/micro_ros/odometry.hpp>

#include "logger.hpp"
#include "wheel_vel_msg.h"

logger logger;

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

static rcl_allocator_t allocator;
static rclc_support_t support;

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

void wheel_vel_callback(const void* arg)
{
	const auto* wheel_vel_msg
		= reinterpret_cast<const struct wheel_vel_msg*>(arg);
	logger.log(
		"wheel_vel_callback wheel velocities: [%.2f], [%.2f], [%.2f], [%.2f]",
		wheel_vel_msg->operator[](0), wheel_vel_msg->operator[](1),
		wheel_vel_msg->operator[](2), wheel_vel_msg->operator[](3));
}

void micro_ros(void* arg)
{
	init(arg);

	rcl_node_t node;
	rclc_node_init_default(&node, "micro_ros_node", "", &support);

	logger.init(&node);

	// TODO: maybe encapsulate as a struct

	auto odometry_exe
		= rclc_executor_get_zero_initialized_executor();
	odometry_init(&odometry_exe, &node, &support, &allocator);

	auto interpolation_exe = rclc_executor_get_zero_initialized_executor();
	interpolation_init(&interpolation_exe, &node, &support, &allocator);

	rcl_subscription_t sub_wheel_vel;
	rclc_subscription_init_default(
		&sub_wheel_vel, &node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray),
		"wheel_vel");
	auto action_exe = rclc_executor_get_zero_initialized_executor();
	rclc_executor_init(&action_exe, &support.context, 1, &allocator);

	wheel_vel_msg msg;
	rclc_executor_add_subscription(&action_exe, &sub_wheel_vel, &msg.msg,
								   &wheel_vel_callback, ON_NEW_DATA);

	logger.log("debug: starting the loop");
	for (;;) {
		rclc_executor_spin_some(&odometry_exe, RCL_MS_TO_NS(1));
		rclc_executor_spin_some(&interpolation_exe, RCL_MS_TO_NS(1));
		rclc_executor_spin_some(&action_exe, RCL_MS_TO_NS(1));
	}
}
}
