#pragma once

#include <rcl/node.h>
#include <rcl/publisher.h>
#include <rclc/publisher.h>
#include <std_msgs/msg/string.h>

struct logger
{
	void init(rcl_node_t* node)
	{
		rclc_publisher_init_default(
			&pub_logger, node,
			ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String), "logger");
	}

	void log(const char* msg)
	{
		std_msgs__msg__String log_msg{const_cast<char*>(msg), strlen(msg),
									  sizeof(msg)};
		[[ maybe_unused ]] auto ret = rcl_publish(&pub_logger, &log_msg, NULL);
	}

	rcl_publisher_t pub_logger;
};

static logger logger;
