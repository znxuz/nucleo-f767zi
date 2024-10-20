#include <experimental/source_location>
#include "nucleo_144/micro_ros/logger.hpp"

extern logger logger;

void rcl_ret_check(rcl_ret_t ret_code,
                   const std::experimental::source_location location =
                       std::experimental::source_location::current()) {
  if (ret_code) {
    logger.log("Failed status on line %d: %d in %s",
               static_cast<int>(location.line()), static_cast<int>(ret_code),
               location.file_name());
  }
}
