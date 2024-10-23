#pragma once

#include <optional>
#include <string_view>

inline constexpr std::string_view STR_ENC_DELTA_RAD =
    "wheel encoder delta in radian";
inline constexpr std::string_view STR_VEL_SP = "wheel velocity setpoint";

enum class WheelDataType { ENC_DELTA_RAD, VEL_SP };

inline std::optional<WheelDataType> parse_wheel_data_type(std::string_view sv) {
  if (sv == STR_ENC_DELTA_RAD) {
    return WheelDataType::ENC_DELTA_RAD;
  } else if (sv == STR_VEL_SP) {
    return WheelDataType::VEL_SP;
  } else {
    return {};
  }
}

inline std::optional<std::string_view> to_string(WheelDataType type) {
  switch (type) {
    case WheelDataType::ENC_DELTA_RAD:
      return STR_ENC_DELTA_RAD;
    case WheelDataType::VEL_SP:
      return STR_VEL_SP;
  }

  return {};
}

inline bool operator==(const WheelDataType& e, std::string_view str) {
  switch (e) {
    case WheelDataType::ENC_DELTA_RAD:
      return str == STR_ENC_DELTA_RAD;
    case WheelDataType::VEL_SP:
      return str == STR_VEL_SP;
  }
  return false;
}

inline bool operator==(std::string_view str, const WheelDataType& e) {
  return e == str;
}
