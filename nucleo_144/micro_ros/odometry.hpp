#pragma once

#include <rcl/allocator.h>
#include <rcl/node.h>
#include <rclc/executor.h>
#include <rclc/executor_handle.h>

extern "C" {
rclc_executor_t* odometry_init(const rcl_node_t* node, rclc_support_t* support,
                               const rcl_allocator_t* allocator);
}
