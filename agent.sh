#!/usr/bin/env zsh

[[ -z "$ROS_DISTRO" ]] && echo "ros not sourced" >&2 && exit 1

cd $HOME/code/microros_ws &&
	source /opt/ros/jazzy-base/setup.zsh &&
	source install/local_setup.zsh &&
	ROS_DOMAIN_ID=42 ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyACM0
