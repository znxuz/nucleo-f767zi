#pragma once

#include <rcl/types.h>

#include <experimental/source_location>

void rcl_ret_check(
    rcl_ret_t ret_code, const std::experimental::source_location location =
                            std::experimental::source_location::current());
