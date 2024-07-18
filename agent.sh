#!/usr/bin/env bash

docker run -it --rm --net=host --privileged microros/micro-ros-agent:jazzy serial --dev /dev/ttyACM0
