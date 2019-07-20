# QtScrcpy 

![build state](https://img.shields.io/badge/build-passing-brightgreen.svg)
![license](https://img.shields.io/badge/license-Apache2.0-blue.svg)
![release](https://img.shields.io/badge/release-v1.0.1-brightgreen.svg)


QtScrcpy can connect to Android devices via USB (or via TCP/IP) for display and control. No root privileges are required.

A single application can support up to 16 Android devices to connect at the same time. Oot permission.

Supports three major desktop platforms, GNU/Linux, Windows and MacOS.

![win](screenshot/win.png)

![mac](screenshot/mac.jpg)

![linux](screenshot/ubuntu.png)

QtScrcpy is based on [Genymobile's](https://github.com/Genymobile) [scrcpy](https://github.com/Genymobile/scrcpy) project and is very grateful to him.

The difference between QtScrcpy and the original scrcpy is as follows:

keys|scrcpy|QtScrcpy
--|:--:|:--:
ui|sdl|qt
video decode|ffmpeg|ffmpeg
video render|sdl|opengl
base tool|c++|Qt
language|C|C++
style|sync|asyn
build|meson+gradle|Qt Creator

- It's very easy to customize your interface with Qt
- Asynchronous programming of Qt-based signal slot mechanism improves performance
- Easy for novices to learn


## Learn

If you are interested in it and want to learn how it works and feel that you can't get started, you can choose to purchase my recorded video lessons.
It details the development architecture and development process of the entire software, and takes you to develop QtScrcpy from scratch.：

course introduction：[https://blog.csdn.net/rankun1/article/details/87970523](https://blog.csdn.net/rankun1/article/details/87970523)

Or you can join my QtScrcpy qq group and exchange ideas with like-minded friends.：

Group number：901736468


## Requirements
The Android part requires at least API 21 (Android 5.0).

Make sure you enabled [adb debugging][enable-adb] on your device(s).

[enable-adb]: https://developer.android.com/studio/command-line/adb.html#Enabling


## Download

[gitee-download]: https://gitee.com/Barryda/QtScrcpy/releases
[github-download]: https://github.com/barry-ran/QtScrcpy/releases

### Windows
For Windows, for simplicity, prebuilt archives with all the dependencies (including adb) are available:

 - [`QtScrcpy`][github-download]

or you can [build it by yourself](#Build)

### Mac OS
For Mac OS, for simplicity, prebuilt archives with all the dependencies (including adb) are available:

- [`QtScrcpy`][github-download]

or you can [build it by yourself](#Build)

### Linux
you can [build it by yourself](#Build)(just ubuntu test)


## Run

Connect to your Android device on your computer, then run the program and click the button below to connect to the Android device.

![run](screenshot/run.png)

## Interface button introduction：

- Startup configuration: function parameter settings before starting the service    

    You can set the bit rate, resolution, recording format, and video save path of the local recorded video.

    - Recording only in the background: Starting the service is not realistic, just recording the Android device screen
    - Window Top: Android device video window top display
    - Close screen: automatically turn off the Android device screen to save power after starting the service
    - Use reverse: service startup mode, service startup failure error more than one device can remove this check to try to connect
    

- Refresh device list: Refresh the currently connected device
- Start the service: connect to the Android device
- Stop service: disconnect from Android device
- Stop all services: disconnect all connected Android devices
- Get device ip: Get the IP address of the Android device and update it to the "Wireless" area for easy wireless connection.
- Start adbd: Start the adbd service of the Android device, you must start it before the wireless connection.
- Wireless connection: Connect to Android devices wirelessly
- Wireless disconnect: Disconnect wirelessly connected Android devices
- adb command line: convenient to execute custom adb commands (currently does not support blocking commands, such as shell)


## The main function
- Display Android device screens in real time
Real-time mouse and keyboard control Android device
- Screen recording
- Wireless connections
- Supports up to 16 device connections (can be added if PC performance allows, you need to compile it yourself)
- full-screen display
- Window topping
- Install apk: drag and drop apk to the video window to install
- Transfer files: Drag files to the video window to send files to Android devices
- Background recording: record only, no display interface


## Why develop QtScrcpy?
There are several reasons for this, and the proportions are arranged from large to small:
1. In the process of learning Qt, you need a project to combat
2. It has audio and video related skills and is very interested in audio and video.
3. It has Android development skills, it’s a bit rusty for a long time, you need to consolidate it.
4. Found scrcpy, decided to re-entamrate with the new technology stack (C++ + Qt + Opengl + ffmpeg)


## Build
Try to provide all the dependencies and make it easy to compile.

### PC client
1. Set up the Qt development environment on the target platform (Qt >= 5.9.7, vs >= 2015 (not support mingw))
2. Clone the project
3. Open the project root directory all.pro with QtCreator
4. Compile and run

### Android (If you do not need to modify the requirements, you can use the built-in scrcpy-server.jar directly)
1. Set up an Android development environment on the target platform
2. Open the server project in the project root directory using Android Studio
3. Build it
4. After compiling apk, rename it to scrcpy-server.jar and replace third_party/scrcpy-server.jar.

## Licence
Since it is based on scrcpy, respect its Licence

    Copyright (C) 2018 Genymobile

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

## About the author

[Barry CSDN](https://blog.csdn.net/rankun1)

An ordinary programmer, working mainly in C++ for desktop client development, graduated from Shandong for more than a year of steel simulation education software, and later moved to Shanghai to work in security, online education related fields, familiar with audio and video. I have an understanding of audio and video fields such as voice calls, live education, video conferencing and other related solutions. At the same time have android, linux server and other development experience.
