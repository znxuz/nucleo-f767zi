#pragma once

#include <cstring>
#include <std_msgs/msg/float64_multi_array.h>

struct wheel_vel_msg
{
	wheel_vel_msg()
	{
		std_msgs__msg__Float64MultiArray__init(&this->msg);

		this->msg.layout.data_offset = 0;
		this->dim.label.data = const_cast<char*>(label);
		this->dim.label.size = strlen(label);
		// set up the array dimension
		this->dim.size = this->size; // number of elements in the array
		this->dim.stride = this->stride;
		this->msg.layout.dim.data = &this->dim;
		this->msg.layout.dim.size = 1;
		this->msg.layout.dim.capacity = 1;

		this->msg.data.data = new double[this->dim.size]{};
		this->msg.data.size = this->dim.size;
		this->msg.data.capacity = this->dim.size;
	}

	wheel_vel_msg(double o0,double o1, double o2, double o3) : wheel_vel_msg()
	{
		this->msg.data.data[0] = o0;
		this->msg.data.data[1] = o1;
		this->msg.data.data[2] = o2;
		this->msg.data.data[3] = o3;
	}

	~wheel_vel_msg()
	{
		delete[] this->msg.data.data;
	}

	double& operator[](size_t i)
	{
		return this->msg.data.data[i];
	}

	const double& operator[](size_t i) const
	{
		return const_cast<wheel_vel_msg*>(this)->operator[](i);
	}

	std_msgs__msg__Float64MultiArray msg;
	std_msgs__msg__MultiArrayDimension dim;
	const uint8_t size = 4;
	const uint8_t stride = 1;
	const char *label = "mecanum wheel velocity array";
};
