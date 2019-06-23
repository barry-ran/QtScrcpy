# QtScrcpy 

![build state](https://img.shields.io/badge/build-passing-brightgreen.svg)
![license](https://img.shields.io/badge/license-Apache2.0-blue.svg)
![release](https://img.shields.io/badge/release-v1.0.1-brightgreen.svg)


此应用程序提供USB(或通过TCP/IP)连接的Android设备的显示和控制。它不需要任何root访问权限。

单个应用程序最多支持16个安卓设备同时连接。

它适用于GNU/Linux，Windows和MacOS。

![win](screenshot/win.png)

![mac](screenshot/mac.jpg)

![linux](screenshot/ubuntu.png)

基于[Genymobile](https://github.com/Genymobile)的[scrcpy](https://github.com/Genymobile/scrcpy)项目进行复刻，重构，非常感谢他。QtScrcpy和原版scrcpy区别如下：

关键点|scrcpy|QtScrcpy
--|:--:|:--:
界面|sdl|qt
视频解码|ffmpeg|ffmpeg
视频渲染|sdl|opengl
跨平台基础设施|自己封装|Qt提供
编程语言|C|C++
编程方式|同步|异步
控制方式|单点触控|单点/多点触控
编译方式|meson+gradle|Qt Creator

- 使用Qt可以非常容易的定制自己的界面
- 基于Qt的信号槽机制的异步编程提高性能
- 方便新手学习
- 增加多点触控支持


## 学习它
如果你对它感兴趣，想学习它的实现原理而又感觉无从下手，可以选择购买我录制的视频课程，
里面详细介绍了整个软件的开发架构以及开发流程，带你从无到有的开发QtScrcpy：

课程介绍：[https://blog.csdn.net/rankun1/article/details/87970523](https://blog.csdn.net/rankun1/article/details/87970523)

或者你也可以加入我的QtScrcpy群，和志同道合的朋友一块互相交流技术：

群号：901736468


## 要求
Android部分至少需要API 21（Android 5.0）。

确保在您的设备上[启用了adb调试][enable-adb]。

[enable-adb]: https://developer.android.com/studio/command-line/adb.html#Enabling


## 下载这个软件

### Windows

对于windows平台，你可以直接使用我编译好的可执行程序:

 - 国内下载 [`QtScrcpy-win32-v1.0.1.zip`][gitee-win32]
 - 国外下载 [`QtScrcpy-win32-v1.0.1.zip`][github-win32]

 

 
[gitee-win32]: https://files.gitee.com/group1/M00/08/37/PaAvDF0O98SAZd-KAf-R_j8Fei8293.zip?token=677250295122144e72ff23de768dbb3a&ts=1561262188&attname=QtScrcpy-win32-v1.0.1.zip&disposition=attachment

[github-win32]: https://github.com/barry-ran/QtScrcpy/releases/download/v1.0.1/QtScrcpy-win32-v1.0.1.zip


你也可以[自己编译](#如何编译)

### Mac OS

对于Mac OS平台，你可以直接使用我编译好的可执行程序:

- 国内下载 [`QtScrcpy-mac64-v1.0.1.zip`][gitee-mac64]
- 国外下载 [`QtScrcpy-mac64-v1.0.1.zip`][github-mac64]

[gitee-mac64]: https://files.gitee.com/group1/M00/08/37/PaAvDF0O98mAEJDJAlpAtBTOOsE526.dmg?token=3fd6b4f6b255a2e0e0362b7e40935ee5&ts=1561262231&attname=QtScrcpy-mac64-v1.0.1.dmg&disposition=attachment

[github-mac64]: https://github.com/barry-ran/QtScrcpy/releases/download/v1.0.1/QtScrcpy-mac64-v1.0.1.zip

你也可以[自己编译](#如何编译)

### Linux

目前只提供了windows和mac平台的可执行程序，如果需要linux平台的可执行程序，

您通常需要[自己编译](#如何编译)。别担心，这并不难。

目前只在ubuntu上测试过

## 运行

在你的电脑上接入Android设备，然后运行程序，按顺序点击如下按钮即可连接到Android设备

![运行](screenshot/run.png)

## 界面按钮介绍：

- 启动配置：启动服务前的功能参数设置    

    分别可以设置本地录制视频的比特率、分辨率、录制格式、录像保存路径等。

    - 仅后台录制：启动服务不现实界面，只是录制Android设备屏幕
    - 窗口置顶：Android设备视频窗口置顶显示
    - 自动息屏：启动服务以后，自动关闭Android设备屏幕节省电量
    - 使用reverse：服务启动模式，出现服务启动失败报错more than one device可以去掉这个勾选尝试连接
    

- 刷新设备列表：刷新当前连接的设备
- 启动服务：连接到Android设备
- 停止服务：断开与Android设备的连接
- 停止所有服务：断开所有已连接的Android设备
- 获取设备ip：获取到Android设备的ip地址，更新到“无线”区域中，方便进行无线连接
- 启动adbd：启动Android设备的adbd服务，无线连接之前，必须要启动。
- 无线连接：使用无线方式连接Android设备
- 无线断开：断开无线方式连接的Android设备
- adb命令行：方便执行自定义adb命令（目前不支持阻塞命令，例如shell）


## 主要功能
- 实时显示Android设备屏幕
- 实时键鼠控制Android设备
- 屏幕录制
- 无线连接
- 最多支持16台设备连接（PC性能允许的情况下可以增加，需要自己编译）
- 全屏显示
- 窗口置顶
- 安装apk：拖拽apk到视频窗口即可安装
- 传输文件：拖拽文件到视频窗口即可发送文件到Android设备
- 后台录制：只录制，不显示界面


## 为什么开发QtScrcpy？
综合起来有以下几个原因，比重从大到小排列：
1. 学习Qt的过程中需要一个项目实战一下
2. 本身具有音视频相关技能，对音视频很感兴趣
3. 本身具有Android开发技能，好久没用有点生疏，需要巩固一下
4. 发现了scrcpy，决定用新的技术栈（C++ + Qt + Opengl + ffmpeg）复刻一下


## 如何编译
尽量提供了所有依赖资源，方便傻瓜式编译：
- 目标平台上搭建Qt开发环境(QtCreator 5.9.7+)
- 克隆该项目
- 使用QtCreator打开项目根目录all.pro
- 编译，运行即可


## Licence
由于是复刻的scrcpy，尊重它的Licence

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

## 关于作者

[Barry的CSDN](https://blog.csdn.net/rankun1)

一枚普通的程序员，工作中主要使用C++进行桌面客户端开发，一毕业在山东做过一年多钢铁仿真教育软件，后来转战上海先后从事安防，在线教育相关领域工作，对音视频比较熟悉，对音视频领域如语音通话，直播教育，视频会议等相关解决方案有所了解。同时具有android，linux服务器等开发经验。