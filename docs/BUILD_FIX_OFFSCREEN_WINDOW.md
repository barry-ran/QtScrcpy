# 修复：副屏移除后投屏窗口变成"幽灵窗口"

## 问题现象

当 QtScrcpy 的投屏窗口（`Phone-<serial>`）位于副屏上时，如果**拔掉副屏**
（或副屏被系统移除、切换扩展屏配置），会出现以下情况：

- 进程仍在运行，任务管理器中可见 `QtScrcpy.exe`
- 窗口状态为"可见"（`IsWindowVisible == true`），未最小化
- **主屏幕上完全看不到窗口**，任务栏也没有入口
- 重启程序后，窗口依然出现在原副屏坐标处（屏幕外），问题不消失

## 根本原因

Windows 的窗口位置以**跨所有显示器的虚拟桌面绝对坐标**保存：原点 `(0,0)`
固定为主屏左上角，其他屏幕按显示设置中的相对排布向四周延伸。

副屏位于主屏右侧时，其上的窗口坐标类似 `X = 3333`。副屏移除后：

1. 系统只剩主屏（例如 `0,0 - 1707x1067`），但**窗口坐标不会被系统自动回收**
2. QtScrcpy 的 `VideoForm` **没有任何显示器变化监听**
3. `moveCenter()` 仅在视频尺寸变化（`updateShowSize()`）时被调用，
   插拔显示器不会触发尺寸变化
4. `showEvent()` 也未做坐标越界检查

因此窗口坐标原样保留在物理上不存在的区域，形成"幽灵窗口"。

> 注：若程序把窗口几何信息持久化到配置，下次启动还会读回该坐标，
> 所以重启程序同样无法恢复。

## 修改内容

修改文件：

- `QtScrcpy/ui/videoform.h`
- `QtScrcpy/ui/videoform.cpp`

新增两个私有方法：

| 方法 | 作用 |
|:--|:--|
| `bool isRectOnAnyScreen(const QRect &rect) const` | 判断矩形是否与任一屏幕的可用区域相交 |
| `void ensureOnScreen()` | 窗口完全越界时，将其移回主屏并居中 |

接入两个触发点：

1. **显示器增删信号**：连接 `QGuiApplication::screenAdded` /
   `screenRemoved`，在屏幕配置变化后做多次延迟检查（300ms / 1s / 2s），
   以等待 Qt 与系统完成屏幕布局更新（Windows 上二者并不完全同步）
2. **窗口显示时**：在 `VideoForm::showEvent()` 中调用，
   处理"上次退出时窗口位于副屏、本次启动副屏已不存在"的情况

### 实现要点

- 使用 `intersects()` 而非 `contains()` 判定：窗口只要有部分可见即视为在屏内，
  保留用户把窗口放在屏幕边缘（部分露出）的合法用法
- 使用 `frameGeometry()` 而非 `geometry()`：前者包含窗口边框，更贴近窗口
  实际占用的屏幕区域
- 越界恢复时**不调用 `moveCenter()`**：因为 `getScreenRect()` 会优先取窗口
  当前所属的 `QScreen`，而窗口此时正位于已失效的屏幕区域，可能得到错误的
  目标位置；因此显式取 `primaryScreen()` 的可用区域计算目标坐标
- 全屏状态下直接跳过，交由系统处理

## 构建方法

### 依赖

| 组件 | 版本要求 | 说明 |
|:--|:--|:--|
| Visual Studio | 2019 / 2022（含 MSVC 与 Windows SDK） | 需要 C++ 桌面开发负载 |
| Qt | 5.15.x（MSVC 64-bit） | 需包含 `Widgets` / `Network` / `Multimedia` |
| CMake | >= 3.19 | 官方构建脚本要求 |
| QtScrcpyCore | 子模块 | 见下方初始化说明 |

### 初始化子模块

`.gitmodules` 中的子模块地址使用 SSH 形式。**若你未配置 GitHub SSH key**，
请改用 HTTPS 拉取：

```bash
git clone --depth 1 https://github.com/barry-ran/QtScrcpy.git
cd QtScrcpy
git clone --depth 1 https://github.com/barry-ran/QtScrcpyCore.git QtScrcpy/QtScrcpyCore
```

### 编译

```bash
set ENV_QT_PATH=<你的 Qt 安装路径，例如 D:\Qt\Qt5.15.2\5.15.2>
cd ci\win
build_for_win.bat Release x64
```

产物输出到 `output/x64/Release/QtScrcpy.exe`。

> 提示：`build_for_win.bat` 中 `qt_cmake_path` 按 `%ENV_QT_PATH%\msvc2019_64\lib\cmake\Qt5`
> 拼接。若你的 Qt 目录结构不同（例如 `msvc2019_64` 改为其他名称），
> 请相应调整该变量。

### 直接使用 CMake

```bash
cmake -DCMAKE_PREFIX_PATH=<Qt路径>/lib/cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -G "Visual Studio 17 2022" -A x64 .
cmake --build . --config Release -j 8
```

## 验证方式

1. 启动 QtScrcpy 并建立投屏，使 `Phone-<serial>` 窗口出现
2. 把该窗口拖到副屏
3. 拔掉副屏（或在显示设置中将该屏设为"断开连接"）
4. **预期**：窗口自动回到主屏并居中

同时可观察日志输出（`qWarning`）：

```
window is off-screen, moving back to primary screen. rect = QRect(3333,100,374,832)
window recovered to QPoint(666,117)
```

## 已知限制

- 若 Qt 因驱动或平台原因**未触发** `screenRemoved` 信号，自动回收不会执行。
  代码中已用多次延迟重试缓解，但仍无法覆盖信号完全丢失的情形。
- 该修复针对窗口越界这一具体问题，未处理其他多屏相关行为
  （如窗口跨屏、DPI 变化等）。
