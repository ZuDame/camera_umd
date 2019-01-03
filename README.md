Old ROS Webcam Drivers
======================

This repository contains `uvc_camera`, a Video4Linux-based webcam driver for ROS. It is **deprecated**,
and it has been replaced by [libuvc_camera](https://github.com/ktossell/libuvc_ros) ([wiki](http://wiki.ros.org/libuvc_camera)), a cross-platform driver
for USB Video Class cameras. Please consider using `libuvc_camera` or another camera driver if your camera
supports the USB Video Class specification (as most webcams and several machine vision cameras do).

Documentation for this package can be found at [its wiki page](http://wiki.ros.org/uvc_camera).
###

安装此包需要配置文件,libv412-dev(若不安装，编译时会报错--libv4l2.h,没有这个文件)
################
sudo apt-get install libv4l-dev

#########
rosrun uvc_camera uvc_camera_node

rosrun image_viewer image_viewer image:=/image_raw

#########
