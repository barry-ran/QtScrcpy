@echo off

echo=
echo=
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

:: 从环境变量获取必要参数
:: example: D:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat
set vcvarsall="%ENV_VCVARSALL%"
:: example: D:\Qt\Qt5.12.5\5.12.5
set qt_msvc_path="%ENV_QT_PATH%"

echo ENV_VCVARSALL %ENV_VCVARSALL%
echo ENV_QT_PATH %ENV_QT_PATH%

:: 获取脚本绝对路径
set script_path=%~dp0
:: 进入脚本所在目录,因为这会影响脚本中执行的程序的工作目录
set old_cd=%cd%
cd /d %~dp0

:: 启动参数声明
set cpu_mode=x86
set build_mode=debug
set errno=1

echo=
echo=
echo ---------------------------------------------------------------
echo check build param[debug/release x86/x64]
echo ---------------------------------------------------------------

:: 编译参数检查 /i忽略大小写
if /i "%1"=="debug" (
    set build_mode=debug
    goto build_mode_ok
)
if /i "%1"=="release" (
    set build_mode=release
    goto build_mode_ok
)
echo error: unkonow build mode -- %1
goto return
:build_mode_ok

if /i "%2"=="x86" (
    set cpu_mode=x86
)
if /i "%2"=="x64" (
    set cpu_mode=x64
)

:: 提示
echo current build mode: %build_mode% %cpu_mode%

:: 环境变量设置
if /i %cpu_mode% == x86 (
    set qt_msvc_path=%qt_msvc_path%\msvc2017\bin
) else (
    set qt_msvc_path=%qt_msvc_path%\msvc2017_64\bin
)
set PATH=%qt_msvc_path%;%PATH%

:: 注册vc环境
if /i %cpu_mode% == x86 (
    call %vcvarsall% %cpu_mode%
) else (
    call %vcvarsall% %cpu_mode%
)

if not %errorlevel%==0 (
    echo "vcvarsall not find"
    goto return
)

echo=
echo=
echo ---------------------------------------------------------------
echo begin qmake build
echo ---------------------------------------------------------------

:: 删除输出目录
set output_path=%script_path%..\..\output\win\%cpu_mode%\%build_mode%
if exist %output_path% (          
    rmdir /q /s %output_path%
)
:: 删除临时目录
set temp_path=%script_path%..\temp
if exist %temp_path% (          
    rmdir /q /s %temp_path%
)
md %temp_path%
cd %temp_path%

set qmake_params=-spec win32-msvc
if /i %build_mode% == debug (
    set qmake_params=%qmake_params% "CONFIG+=debug" "CONFIG+=qml_debug"
) else (
    set qmake_params=%qmake_params% "CONFIG+=qtquickcompiler"
)

:: qmake ../../all.pro -spec win32-msvc "CONFIG+=debug" "CONFIG+=qml_debug"
qmake ../../all.pro %qmake_params%
if not %errorlevel%==0 (
    echo "qmake failed"
    goto return
)

:: nmake
:: jom是qt的多进程nmake工具
..\win\jom -j8
if not %errorlevel%==0 (
    echo "nmake failed"
    goto return
)

echo=
echo=
echo ---------------------------------------------------------------
echo finish!!!
echo ---------------------------------------------------------------

set errno=0

:return
cd %old_cd%
exit /B %errno%