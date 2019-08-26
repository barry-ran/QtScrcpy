@echo off
set VsDevCmd="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat"
set qt_msvc_path="D:\Qt\Qt5.12.4\5.12.4\msvc2017\bin"

:: 获取脚本绝对路径
set script_path=%~dp0
:: 进入脚本所在目录,因为这会影响脚本中执行的程序的工作目录
set old_cd=%cd%
cd /d %~dp0

:: 启动参数声明
set debug_mode="false"

echo=
echo=
echo ---------------------------------------------------------------
echo 检查编译参数[debug/release]
echo ---------------------------------------------------------------

:: 编译参数检查 /i忽略大小写
if /i "%1"=="debug" (
    set debug_mode="true"
    goto param_ok
)
if /i "%1"=="release" (
    set debug_mode="false"
    goto param_ok
)

echo "waring: unkonow build mode -- %1, default release"
set debug_mode="false"
goto param_ok

:param_ok

:: 提示
if /i %debug_mode% == "true" (
    echo 当前编译版本为debug版本
) else (
    echo 当前编译版本为release版本
)

:: 环境变量设置
set build_path=%script_path%build
set PATH=%qt_msvc_path%;%PATH%

call %VsDevCmd%

if not %errorlevel%==0 (
    echo "VsDevCmd not find"
    goto return
)

echo=
echo=
echo ---------------------------------------------------------------
echo 开始qmake编译
echo ---------------------------------------------------------------

if exist %build_path% (          
    rmdir /q /s %build_path%
)
md %build_path%
cd %build_path%

set qmake_params=-spec win32-msvc

if /i %debug_mode% == "true" (
    set qmake_params=%qmake_params% "CONFIG+=debug" "CONFIG+=qml_debug"
) else (
    set qmake_params=%qmake_params% "CONFIG+=qtquickcompiler"
)

:: qmake ../all.pro -spec win32-msvc "CONFIG+=debug" "CONFIG+=qml_debug"
qmake ../all.pro %qmake_params%

nmake

if not %errorlevel%==0 (
    echo "qmake build failed"
    goto return
)

echo=
echo=
echo ---------------------------------------------------------------
echo 完成！
echo ---------------------------------------------------------------

:return
cd %old_cd%