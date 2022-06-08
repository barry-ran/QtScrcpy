@echo off

echo=
echo=
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

:: 从环境变量获取必要参数
:: example: D:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat
set vcvarsall="%ENV_VCVARSALL%"
:: 例如 d:\a\QtScrcpy\Qt\5.12.7
set qt_msvc_path="%ENV_QT_PATH%"
:: 设置了VCINSTALLDIR，windeployqt会自动copy vc_redist.x**.exe(vcruntime dll安装包)
:: set VCINSTALLDIR="%ENV_VCINSTALL%"

echo ENV_VCVARSALL %ENV_VCVARSALL%
echo ENV_QT_PATH %ENV_QT_PATH%

:: 获取脚本绝对路径
set script_path=%~dp0
:: 进入脚本所在目录,因为这会影响脚本中执行的程序的工作目录
set old_cd=%cd%
cd /d %~dp0

:: 启动参数声明
set cpu_mode=x86
set publish_dir=%2
set errno=1

if /i "%1"=="x86" (
    set cpu_mode=x86
)
if /i "%1"=="x64" (
    set cpu_mode=x64
)

:: 提示
echo current build mode: %cpu_mode%
echo current publish dir: %publish_dir%

:: 环境变量设置
set adb_path=%script_path%..\..\QtScrcpy\QtScrcpyCore\src\third_party\adb\win\*.*
set jar_path=%script_path%..\..\QtScrcpy\QtScrcpyCore\src\third_party\scrcpy-server
set keymap_path=%script_path%..\..\keymap
set config_path=%script_path%..\..\config

if /i %cpu_mode% == x86 (
    set publish_path=%script_path%%publish_dir%\
    set release_path=%script_path%..\..\output\x86\RelWithDebInfo
    set qt_msvc_path=%qt_msvc_path%\msvc2019\bin
) else (
    set publish_path=%script_path%%publish_dir%\
    set release_path=%script_path%..\..\output\x64\RelWithDebInfo
    set qt_msvc_path=%qt_msvc_path%\msvc2019_64\bin
)
set PATH=%qt_msvc_path%;%PATH%

:: 注册vc环境(注册以后，windeployqt会把vc_redist复制过来（vcruntime安装包）)
if /i %cpu_mode% == x86 (
    call %vcvarsall% %cpu_mode%
) else (
    call %vcvarsall% %cpu_mode%
)

if exist %publish_path% (
    rmdir /s/q %publish_path%
)

:: 复制要发布的包
xcopy %release_path% %publish_path% /E /Y
xcopy %adb_path% %publish_path% /Y
xcopy %jar_path% %publish_path% /Y
xcopy %keymap_path% %publish_path%keymap\ /E /Y
xcopy %config_path% %publish_path%config\ /E /Y

:: 添加qt依赖包
windeployqt %publish_path%\QtScrcpy.exe

:: 删除多余qt依赖包
rmdir /s/q %publish_path%\iconengines
rmdir /s/q %publish_path%\translations

:: 截图功能需要qjpeg.dll
del %publish_path%\imageformats\qgif.dll
del %publish_path%\imageformats\qicns.dll
del %publish_path%\imageformats\qico.dll
::del %publish_path%\imageformats\qjpeg.dll
del %publish_path%\imageformats\qsvg.dll
del %publish_path%\imageformats\qtga.dll
del %publish_path%\imageformats\qtiff.dll
del %publish_path%\imageformats\qwbmp.dll
del %publish_path%\imageformats\qwebp.dll

:: 删除vc_redist，自己copy vcruntime dll
if /i %cpu_mode% == x86 (
    del %publish_path%\vc_redist.x86.exe
) else (
    del %publish_path%\vc_redist.x64.exe
)

:: copy vcruntime dll
if /i %cpu_mode% == x64 (
    cp "C:\Windows\System32\msvcp140_1.dll" %publish_path%\msvcp140_1.dll
    cp "C:\Windows\System32\msvcp140.dll" %publish_path%\msvcp140.dll
    cp "C:\Windows\System32\vcruntime140.dll" %publish_path%\vcruntime140.dll
    :: 只有x64需要
    cp "C:\Windows\System32\vcruntime140_1.dll" %publish_path%\vcruntime140_1.dll
) else (
    cp "C:\Windows\SysWOW64\msvcp140_1.dll" %publish_path%\msvcp140_1.dll
    cp "C:\Windows\SysWOW64\msvcp140.dll" %publish_path%\msvcp140.dll
    cp "C:\Windows\SysWOW64\vcruntime140.dll" %publish_path%\vcruntime140.dll
    
)

::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\VCRUNTIME140.dll" %publish_path%\VCRUNTIME140.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-runtime-l1-1-0.dll" %publish_path%\api-ms-win-crt-runtime-l1-1-0.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-heap-l1-1-0.dll" %publish_path%\api-ms-win-crt-heap-l1-1-0.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-math-l1-1-0.dll" %publish_path%\api-ms-win-crt-math-l1-1-0.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-stdio-l1-1-0.dll" %publish_path%\api-ms-win-crt-stdio-l1-1-0.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-locale-l1-1-0.dll" %publish_path%\api-ms-win-crt-locale-l1-1-0.dll

echo=
echo=
echo ---------------------------------------------------------------
echo finish!!!
echo ---------------------------------------------------------------

set errno=0

:return
cd %old_cd%
exit /B %errno%