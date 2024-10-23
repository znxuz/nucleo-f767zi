#pragma once

#include <rclc/executor.h>

extern "C" {
rclc_executor_t* encoder_data_exe_init(const rcl_node_t* node,
                                       rclc_support_t* support,
                                       const rcl_allocator_t* allocator);
}
