# QtScrcpy 

[![Financial Contributors on Open Collective](https://opencollective.com/QtScrcpy/all/badge.svg?label=financial+contributors)](https://opencollective.com/QtScrcpy)
![Windows](https://github.com/barry-ran/QtScrcpy/workflows/Windows/badge.svg)
![MacOS](https://github.com/barry-ran/QtScrcpy/workflows/MacOS/badge.svg)
![Ubuntu](https://github.com/barry-ran/QtScrcpy/workflows/Ubuntu/badge.svg)

![license](https://img.shields.io/badge/license-Apache2.0-blue.svg)
![release](https://img.shields.io/github/v/release/barry-ran/QtScrcpy.svg)

<a href="https://tracking.gitads.io/?repo=QtScrcpy"><img src="https://images.gitads.io/QtScrcpy" alt="GitAds"/></a>

[中文介绍](README_zh.md)

QtScrcpy connects to Android devices via USB (or via TCP/IP) for display and control. It does NOT require the root privileges.

A single instance supports up to 16 Android device connections at the same time.

It supports three major platforms: GNU/Linux, Windows and MacOS.

It focuses on:

 - **lightness** (native, displays only the device screen)
 - **performance** (30~60fps)
 - **quality** (1920×1080 or above)
 - **low latency** ([35~70ms][lowlatency])
 - **low startup time** (~1 second to display the first image)
 - **non-intrusiveness** (nothing is left installed on the device)

[lowlatency]: https://github.com/Genymobile/scrcpy/pull/646

![win](screenshot/win.png)

![mac](screenshot/mac.jpg)

![linux](screenshot/ubuntu.png)

## Customized key mapping
You can write your own script to map keyboard and mouse actions to touches and clicks of the mobile phone according to your needs. [Here](docs/KeyMapDes.md) are the rules.

A script for "PUBG mobile" and TikTok mapping is provided by default. Once enabled, you can play the game with your keyboard and mouse as the PC version. or you can use up/down/left/right direction keys to simulate up/down/left/right sliding. You can also write your own mapping files for other games according to [writing rules](docs/KeyMapDes.md). The default key mapping is as follows:

![game](screenshot/game.jpg)

[Here is a video demonstration of playing "PUBG mobile"](http://mp.weixin.qq.com/mp/video?__biz=MzU1NTg5MjYyNw==&mid=100000015&sn=3e301fdc5a364bd16d6207fa674bc8b3&vid=wxv_968792362971430913&idx=1&vidsn=eec329cc13c3e24c187dc9b4d5eb8760&fromid=1&scene=20&xtrack=1&clicktime=1567346543&sessionid=1567346375&subscene=92&ascene=0&fasttmpl_type=0&fasttmpl_fullversion=4730859-zh_CN-zip&fasttmpl_flag=0&realreporttime=1567346543910#wechat_redirect)

Here is the instruction of adding new customized mapping files.

- Write a customized script and put it in the `keymap` directory
- Click `refresh script` to check whether it can be found
- Select your script
- Connect your phone, start service and click `apply`
- Press `~` key (left side of the number key 1) to switch to the custom mapping mode (It can be changed in the script as `switchkey`)
- Press the ~ key again to switch back to normal mode
- (For PUBG and similar games) If you want to drive cars with WASD, you need to check the `single rocker mode` in the game setting.

## Group control
You can control all your phones at the same time.

![](docs/image/group-control.gif)

## Thanks

QtScrcpy is based on [Genymobile's](https://github.com/Genymobile) [scrcpy](https://github.com/Genymobile/scrcpy) project. Thanks

The difference between QtScrcpy and the original scrcpy is as follows:

keys|scrcpy|QtScrcpy
--|:--:|:--:
ui|sdl|qt
video encode|ffmpeg|ffmpeg
video render|sdl|opengl
cross-platform|self implemented|provided by Qt
language|C|C++
style|sync|async
keymap|no custom keymap|support custom keymap
build|meson+gradle|Qt Creator

- It's very easy to customize your GUI with Qt
- Asynchronous programming of Qt-based signal slot mechanism improves performance
- Easy to learn
- Add support for multi-touch


## Learn

If you are interested in it and want to learn how it works but do not know how to get started, you can choose to purchase my recorded video lessons.
It details the development architecture and the development process of the entire software, and help you develop QtScrcpy from scratch.

Course introduction：[https://blog.csdn.net/rankun1/article/details/87970523](https://blog.csdn.net/rankun1/article/details/87970523)

You can join my QQ group for QtScrcpy and exchange ideas with like-minded friends.：

QQ Group number：901736468


## Requirements
Android API >= 21 (Android 5.0).

Make sure you enabled [adb debugging][enable-adb] on your device(s).

[enable-adb]: https://developer.android.com/studio/command-line/adb.html#Enabling


## Download

[gitee-download]: https://gitee.com/Barryda/QtScrcpy/releases
[github-download]: https://github.com/barry-ran/QtScrcpy/releases

### Windows
For Windows, for simplicity, prebuilt archives with all the dependencies (including adb) are available:

 - [`QtScrcpy`][github-download]

or you can [build it by yourself](##Build)

### Mac OS
For Mac OS, for simplicity, prebuilt archives with all the dependencies (including adb) are available:

- [`QtScrcpy`][github-download]

or you can [build it by yourself](##Build)

### Linux
you can [build it by yourself](##Build)(just ubuntu test)


## Run

Connect to your Android device on your computer, then run the program and click the button below to connect to the Android device.

![run](screenshot/run.png)

### Wireless connection steps (ensure that the mobile phone and PC are in the same LAN):
1. Enable USB debugging in developer options on the Android device
2. Connect the Android device to computer via USB
3. Click update device, and you will see that the device number is updated
4. Click get device IP
5. Click start adbd
6. Click wireless connect
7. Click update device again, and another device with IP address will be found. Select this device.
8. Click start service

​	

Note: it is not necessary to keep you Android device connected via USB after you start adbd.

## Interface button introduction：

- Start config: function parameter settings before starting the service    

    You can set the bit rate, resolution, recording format, and video save path of the local recorded video.

    - Background record: the Android device screen is not displayed after starting the service. It is recorded in background.
    - Always on top: the video window for Android device will be kept on the top
    - Close screen: automatically turn off the Android device screen to save power after starting the service
    - Reverse connection: service startup mode. You can uncheck it if you experience connection failure with message `more than one device`
    
- Refresh devices: Refresh the currently connected device
- Start service: connect to the Android device
- Stop service: disconnect from Android device
- Stop all services: disconnect all connected Android devices
- Get device IP: Get the IP address of the Android device and update it to the "Wireless" area for the ease of wireless connection setting.
- Start adbd: Start the adbd service of the Android device. You must start it before the wireless connection.
- Wireless connect: Connect to Android devices wirelessly
- Wireless disconnect: Disconnect wirelessly connected Android devices
- adb command: execute customized adb commands (blocking commands are not supported now, such as shell)


## The main function
- Display Android device screens in real time
- Real-time mouse and keyboard control of Android devices
- Screen recording
- Screenshot to png
- Wireless connection
- Supports up to 16 device connections (the number can be higher if your PC performance allows. You need to compile it by yourself)
- Full-screen display
- Display on the top
- Install apk: drag and drop apk to the video window to install
- Transfer files: Drag files to the video window to send files to Android devices
- Background recording: record only, no display interface
- Copy-paste

    It is possible to synchronize clipboards between the computer and the device, in
    both directions:

    - `Ctrl`+`c` copies the device clipboard to the computer clipboard;
    - `Ctrl`+`Shift`+`v` copies the computer clipboard to the device clipboard;
    - `Ctrl`+`v` _pastes_ the computer clipboard as a sequence of text events (but
    breaks non-ASCII characters).
- Group control

## Shortcuts

 | Action                                 |   Shortcut (Windows)          |   Shortcut (macOS)
 | -------------------------------------- |:----------------------------- |:-----------------------------
 | Switch fullscreen mode                 | `Ctrl`+`f`                    | `Cmd`+`f`
 | Resize window to 1:1 (pixel-perfect)   | `Ctrl`+`g`                    | `Cmd`+`g`
 | Resize window to remove black borders  | `Ctrl`+`x` \| _Double-click¹_ | `Cmd`+`x`  \| _Double-click¹_
 | Click on `HOME`                        | `Ctrl`+`h` \| _Middle-click_  | `Ctrl`+`h` \| _Middle-click_
 | Click on `BACK`                        | `Ctrl`+`b` \| _Right-click²_  | `Cmd`+`b`  \| _Right-click²_
 | Click on `APP_SWITCH`                  | `Ctrl`+`s`                    | `Cmd`+`s`
 | Click on `MENU`                        | `Ctrl`+`m`                    | `Ctrl`+`m`
 | Click on `VOLUME_UP`                   | `Ctrl`+`↑` _(up)_             | `Cmd`+`↑` _(up)_
 | Click on `VOLUME_DOWN`                 | `Ctrl`+`↓` _(down)_           | `Cmd`+`↓` _(down)_
 | Click on `POWER`                       | `Ctrl`+`p`                    | `Cmd`+`p`
 | Power on                               | _Right-click²_                | _Right-click²_
 | Turn device screen off (keep mirroring)| `Ctrl`+`o`                    | `Cmd`+`o`
 | Expand notification panel              | `Ctrl`+`n`                    | `Cmd`+`n`
 | Collapse notification panel            | `Ctrl`+`Shift`+`n`            | `Cmd`+`Shift`+`n`
 | Copy device clipboard to computer      | `Ctrl`+`c`                    | `Cmd`+`c`
 | Paste computer clipboard to device     | `Ctrl`+`v`                    | `Cmd`+`v`
 | Copy computer clipboard to device      | `Ctrl`+`Shift`+`v`            | `Cmd`+`Shift`+`v`

_¹Double-click on black borders to remove them._  

_²Right-click turns the screen on if it was off, presses BACK otherwise._

## TODO
[TODO](docs/TODO.md)

## FAQ
[FAQ](docs/FAQ.md)

## DEVELOP
[DEVELOP](docs/DEVELOP.md)

Everyone is welcome to maintain this project and contribute your own code, but please follow these requirements:
1. pr please mention the dev branch, not the master branch
2. Please rebase dev before mentioning pr
3. pr please submit on the principle of a small number of times (a small function point is recommended to mention a pr)
4. Please keep the code style consistent with the existing style

## Why develop QtScrcpy?
There are several reasons listed as below according to importance (high to low).
1. In the process of learning Qt, I need a real project to try
2. I have some background skill about audio and video and I am interested at them
3. I have some Android development skills. But I have used it for a long time. I want to consolidate it.
4. I found scrcpy and decided to re-make it with the new technology stack (C++ + Qt + Opengl + ffmpeg)


## Build
All the dependencies are provided and it is easy to compile.

### PC client
1. Set up the Qt development environment on the target platform (Qt == 5.15.0, vs == 2017 (mingw not supported))
2. Clone the project
3. Open the project root directory all.pro with QtCreator
4. Compile and run

### Android (If you do not have special requirements, you can directly use the built-in scrcpy-server.jar)

1. Set up an Android development environment on the target platform
2. Open server project in project root with Android Studio
3. The first time you open it, if you do not have the corresponding version of gradle, you will be prompted to find gradle, whether to upgrade gradle and create it. Select Cancel. After canceling, you will be prompted to select the location of the existing gradle. You can also cancel it (it will download automatically).
4. Edit the code as needed, but of course you do n’t need to.
4. After compiling the apk, rename it to scrcpy-server and replace third_party/scrcpy-server.

## Licence
Since it is based on scrcpy, respect its Licence

    Copyright (C) 2020 Barry
    
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

## Contributors

### Code Contributors

This project exists thanks to all the people who contribute. [[Contribute](CONTRIBUTING.md)].
<a href="https://github.com/barry-ran/QtScrcpy/graphs/contributors"><img src="https://opencollective.com/QtScrcpy/contributors.svg?width=890&button=false" /></a>

### Financial Contributors

Become a financial contributor and help us sustain our community. [[Contribute](https://opencollective.com/QtScrcpy/contribute)]

#### Individuals

<a href="https://opencollective.com/QtScrcpy"><img src="https://opencollective.com/QtScrcpy/individuals.svg?width=890"></a>

#### Organizations

Support this project with your organization. Your logo will show up here with a link to your website. [[Contribute](https://opencollective.com/QtScrcpy/contribute)]

<a href="https://opencollective.com/QtScrcpy/organization/0/website"><img src="https://opencollective.com/QtScrcpy/organization/0/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/1/website"><img src="https://opencollective.com/QtScrcpy/organization/1/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/2/website"><img src="https://opencollective.com/QtScrcpy/organization/2/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/3/website"><img src="https://opencollective.com/QtScrcpy/organization/3/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/4/website"><img src="https://opencollective.com/QtScrcpy/organization/4/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/5/website"><img src="https://opencollective.com/QtScrcpy/organization/5/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/6/website"><img src="https://opencollective.com/QtScrcpy/organization/6/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/7/website"><img src="https://opencollective.com/QtScrcpy/organization/7/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/8/website"><img src="https://opencollective.com/QtScrcpy/organization/8/avatar.svg"></a>
<a href="https://opencollective.com/QtScrcpy/organization/9/website"><img src="https://opencollective.com/QtScrcpy/organization/9/avatar.svg"></a>