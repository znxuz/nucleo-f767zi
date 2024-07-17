#!/usr/bin/env bash

tmux splitw 'docker run -it --rm --net=host --privileged microros/micro-ros-agent:jazzy serial --dev /dev/ttyACM0'
