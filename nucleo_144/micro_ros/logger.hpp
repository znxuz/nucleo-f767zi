#pragma once

#include <rcl/node.h>
#include <rcl/publisher.h>
#include <rclc/publisher.h>
#include <std_msgs/msg/string.h>

static constexpr uint8_t MAX_MSG_SIZE = 100;

struct logger
{
	void init(rcl_node_t* node)
	{
		rclc_publisher_init_default(
			&pub_logger, node,
			ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String), "logger");
	}

	void log(const char* format, ...)
	{
		va_list arglist;
		va_start(arglist, format);
		vsnprintf(msg, MAX_MSG_SIZE, format, arglist);
		va_end(arglist);
		msg[MAX_MSG_SIZE - 1] = '\0';

		std_msgs__msg__String log_msg{msg, strlen(msg), MAX_MSG_SIZE};
		[[ maybe_unused ]] auto ret = rcl_publish(&pub_logger, &log_msg, NULL);
	}

	rcl_publisher_t pub_logger;
	char msg[MAX_MSG_SIZE]{};
};

static logger logger;
