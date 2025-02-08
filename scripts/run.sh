#!/usr/bin/env bash

set_scan_legacy()
{
	ros2 topic pub --once /start_scan std_msgs/msg/Bool "data: $1"
}

drive_x()
{
	vel="${1:-0.5}"
	ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
		"{linear: {x:  $vel, y: 0.0, z: 0.0}, angular: { x: 0.0, y: 0.0, z: 0.0}}"
}

drive_y()
{
	vel="${1:-0.5}"
	ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
			"{linear: {x:  0.0, y: $vel, z: 0.0}, angular: { x: 0.0, y: 0.0, z: 0.0}}"
}

turn()
{
	vel="${1:-1}"
	ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
			"{linear: {x:  0.0, y: 0.0, z: 0.0}, angular: { x: 0.0, y: 0.0, z: $vel}}"
}

clear_drive()
{
	ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
			"{linear: {x:  0.0, y: 0.0, z: 0.0}, angular: { x: 0.0, y: 0.0, z: 0.0}}"
	sleep 2
}

start_keyboard_control()
{
	echo "starting the keyboard control..." && ros2 run teleop_twist_keyboard teleop_twist_keyboard
}

toggle_lidar()
{
	ros2 topic pub --once /lidar_enable std_msgs/msg/Bool "{data: $1}"
}

main()
{
	[[ -z "$ROS_DISTRO" ]] && echo "ros not sourced" >&2 && exit 1

	if [[ "$1" = "test" ]]; then
		drive_x 0.5 && sleep 2 && clear_drive
		drive_x -0.5 && sleep 2 && clear_drive

		drive_x 0.5 && sleep 2 && clear_drive
		turn 3.14 && sleep 2 && clear_drive
		drive_x 0.5 && sleep 2 && clear_drive
		turn -3.14 && sleep 2 && clear_drive
	elif [[ "$1" = "lidar" ]]; then
		toggle_lidar "$2"
		# set_scan_legacy "$2"
	else
		start_keyboard_control
	fi
}

main "$@"
