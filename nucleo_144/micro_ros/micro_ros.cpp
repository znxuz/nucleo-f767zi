#include <rcl/allocator.h>
#include <rcl/node.h>
#include <rcl/publisher.h>
#include <rclc/publisher.h>
#include <rclc/types.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rcl/subscription.h>
#include <rclc/executor.h>
#include <rclc/executor_handle.h>
#include <rclc/rclc.h>
#include <rclc/subscription.h>
#include <rcutils/logging.h>
#include <rmw_microros/rmw_microros.h>
#include <rmw_microxrcedds_c/config.h>
#include <uxr/client/transport.h>

#include <std_msgs/msg/char.h>
#include <std_msgs/msg/int8.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/string.h>

#include <cmsis_os2.h>
#include <stm32f7xx_hal.h>

#include <experimental/source_location>

#include "logger.hpp"

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

void int_in_callback(const void* msg)
{
	logger.log("int data in");
}

void timer_callback(rcl_timer_t* timer, int64_t last_call_time)
{
	logger.log("timer callback");
}

void micro_ros(void* arg)
{
	init(arg);

	rcl_node_t node;
	rclc_node_init_default(&node, "micro_ros_node", "", &support);

	logger.init(&node);

	auto first_exe = rclc_executor_get_zero_initialized_executor();
	rclc_executor_init(&first_exe, &support.context, 1, &allocator);

	rcl_subscription_t sub_int_in;
	rclc_subscription_init_default(
	&sub_int_in, &node,
	ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int8), "int_in");
	auto int_msg = std_msgs__msg__Int8();
	rclc_executor_add_subscription(&first_exe, &sub_int_in, &int_msg,
	&int_in_callback, ON_NEW_DATA);

	// auto second_exe = rclc_executor_get_zero_initialized_executor();
	// rclc_executor_init(&second_exe, &support.context, 1, &allocator);
	// auto timer = rcl_get_zero_initialized_timer();
	// const unsigned int timer_timeout = 1000;
	// rclc_timer_init_default2(&timer, &support, RCL_MS_TO_NS(timer_timeout),
	// timer_callback, true);
	// rclc_executor_add_timer(&second_exe, &timer);

	for (;;) {
		rclc_executor_spin_some(&first_exe, RCL_MS_TO_NS(10));
		// rclc_executor_spin_some(&second_exe, RCL_MS_TO_NS(1));

		osDelay(1000);
	}
}

}
