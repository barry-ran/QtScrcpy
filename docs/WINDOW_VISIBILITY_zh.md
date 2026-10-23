# 投屏窗口透明度与自动隐身

连接设备后，右侧工具栏顶部新增两个按钮：

- **透明度**：打开滑块，手动调节投屏窗口和工具栏的不透明度。范围为 10%–100%，数值越低越透明；拖动时即时生效。
- **自动隐身**：点击开启或关闭，开启时按钮高亮。鼠标离开投屏窗口、标题栏和工具栏连续 300 毫秒后，窗口变得几乎不可见；移回实际可操作区域连续停留 300 毫秒后，恢复手动设置的不透明度。

两个设置分别按设备记忆，重新连接同一设备时恢复。自动隐身默认关闭，不透明度默认 100%。

鼠标从画面移到工具栏或透明度弹窗时，会继续保持显示。拖动窗口、调整大小或打开菜单时也应保持显示。正常显示时，窗口外侧有 12 像素的离开容差，避免边缘抖动造成反复切换。

隐身方式参考 AltSnap：将窗口不透明度降到 1/255，保留原位置、窗口层级和鼠标命中区域。因此隐身区域仍然会挡住下面窗口的点击；它不属于点击穿透，也不会断开手机连接。被其他窗口遮挡的区域不会触发恢复。

设置保存在 QtScrcpy 用户配置目录的 `userdata.ini` 中，各设备组下的 `WindowOpacityPercent` 和 `WindowAutoHideEnabled` 两项。滑动调整使用延迟写入，避免每移动一个刻度都写入文件。

本功能使用电脑端窗口与鼠标事件，不需要修改或重新编译 Android 服务端。

## 使用现有 Qt 6 / MinGW 构建

在仓库根目录使用本机已有的 PowerShell 7 执行：

```powershell
$qtKit = 'C:\Qt\6.10.1\mingw_64'
$mingwKit = 'C:\Qt\Tools\mingw1310_64'
.\ci\win\build_mingw.ps1 -QtDirectory $qtKit -MingwDirectory $mingwKit
```

将示例中的 Qt 和 MinGW 路径替换为实际安装位置，并使用与 Qt 套件匹配的编译器。脚本优先查找同一 Qt 安装根目录下的 CMake 和 Ninja，其次查找 PATH；也可通过 `-CMakePath`、`-NinjaPath` 显式指定。构建输出位于 `output\x64\Release`。

创建可独立运行的便携目录：

```powershell
.\ci\win\build_mingw.ps1 -QtDirectory $qtKit -MingwDirectory $mingwKit -PackageDirectory ..\QtScrcpy-qt6-x64-privacy
```

目标目录必须尚不存在，脚本会复制程序、Qt 和 MinGW 运行库、FFmpeg、ADB、配置模板及键位映射。分发时保留整个目录，不要只复制 `QtScrcpy.exe`。

开发验证可增加 `-BuildVisibilityTests`，运行真实投屏窗口、工具栏、OpenGL 合成画面及设置保存的集成测试；这会短暂显示测试窗口并移动鼠标，不需要连接手机。独立窗口控制器测试位于 `tests/windowvisibility`。
