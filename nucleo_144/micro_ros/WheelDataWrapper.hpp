#pragma once

#include <std_msgs/msg/float32_multi_array.h>
#include <std_msgs/msg/float64_multi_array.h>

#include <cmath>
#include <cstring>
#include <type_traits>
#include <array>

#include "WheelDataType.hpp"

inline constexpr uint8_t N_WHEEL = 4;

template <typename T>
concept FltNum = std::is_same_v<T, float> || std::is_same_v<T, double>;

template <typename FltNum>
using MsgType =
    typename std::conditional_t<std::is_same_v<FltNum, float>,
                              std_msgs__msg__Float32MultiArray,
                              std_msgs__msg__Float64MultiArray>;

template <typename MsgType>
inline const char* extract_label(const MsgType& msg) {
  return msg.layout.dim.data->label.data;  // jeez thats a mouthful
}

template <FltNum T, WheelDataType E>
struct WheelDataWrapper {

  WheelDataWrapper() : label{to_string(E).value()} {
    this->msg.layout.data_offset = 0;
    this->dim.label.data = const_cast<char*>(this->label.data());
    this->dim.label.size = this->label.size();
    this->dim.label.capacity = 0; // constant string ptr - not modifiable
    // set up the array dimension
    this->dim.size = this->size;  // number of elements in the array
    this->dim.stride = this->stride;
    this->msg.layout.dim.data = &this->dim;
    this->msg.layout.dim.size = 1;
    this->msg.layout.dim.capacity = 1;

    this->msg.data.data = new T[this->dim.size]{};
    this->msg.data.size = this->dim.size;
    this->msg.data.capacity = this->dim.size;
  }

  WheelDataWrapper(const std::array<T, N_WHEEL>& data) : WheelDataWrapper() {
    std::copy(begin(data), end(data), this->msg.data.data);
  }

  WheelDataWrapper(const WheelDataWrapper&) = delete;
  WheelDataWrapper(WheelDataWrapper&&) = delete;
  WheelDataWrapper& operator=(const WheelDataWrapper&) = delete;
  WheelDataWrapper& operator=(WheelDataWrapper&&) = delete;

  ~WheelDataWrapper() { delete[] this->msg.data.data; }

  T& operator[](size_t i) { return this->msg.data.data[i]; }

  const T& operator[](size_t i) const {
    return const_cast<WheelDataWrapper*>(this)->operator[](i);
  }

  static const rosidl_message_type_support_t* get_msg_type_support() {
    if constexpr (std::is_same_v<T, float>)
      return ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray);
    else
      return ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64MultiArray);
  }

  const std::string_view label;
  const uint8_t size = N_WHEEL;
  const uint8_t stride = 1;

  MsgType<T> msg;
  std_msgs__msg__MultiArrayDimension dim;
};
