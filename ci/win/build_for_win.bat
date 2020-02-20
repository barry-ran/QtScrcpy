@echo off
set vcvarsall="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
set qt_msvc_path="d:\a\QtScrcpy\Qt\5.12.7\"
set build_path_name="../build-win"

:: 获取脚本绝对路径
set script_path=%~dp0
:: 进入脚本所在目录,因为这会影响脚本中执行的程序的工作目录
set old_cd=%cd%
cd /d %~dp0

set build_path=%script_path%%build_path_name%

:: 启动参数声明
set debug_mode="false"
set cpu_mode=x86
set errno=1

echo=
echo=
echo ---------------------------------------------------------------
echo check build param[debug/release x86/x64]
echo ---------------------------------------------------------------

:: 编译参数检查 /i忽略大小写
if /i "%1"=="debug" (
    set debug_mode="true"
)
if /i "%1"=="release" (
    set debug_mode="false"
)

if /i "%2"=="x86" (
    set cpu_mode=x86
)
if /i "%2"=="x64" (
    set cpu_mode=x64
)

:: 提示
if /i %debug_mode% == "true" (
    echo current build mode: debug %cpu_mode%
) else (
    echo current build mode: release %cpu_mode%
)

:: 环境变量设置
if /i %cpu_mode% == x86 (
    set qt_msvc_path=%qt_msvc_path%msvc2017\bin
) else (
    set qt_msvc_path=%qt_msvc_path%msvc2017_64\bin
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

if exist %build_path% (          
    rmdir /q /s %build_path%
)
md %build_path%
cd %build_path%

:: set qmake_params=-spec win32-msvc

if /i %debug_mode% == "true" (
    set qmake_params=%qmake_params% "CONFIG+=debug" "CONFIG+=qml_debug"
) else (
    set qmake_params=%qmake_params% "CONFIG+=qtquickcompiler"
)

:: qmake ../../all.pro -spec win32-msvc "CONFIG+=debug" "CONFIG+=qml_debug"
:: qmake ../../all.pro %qmake_params%
:qmake ../../all.pro

nmake

if not %errorlevel%==0 (
    echo "qmake build failed"
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