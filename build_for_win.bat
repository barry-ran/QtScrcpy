@echo off
set vcvarsall="D:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat"
set qt_msvc_path="D:\Qt\Qt5.12.5\5.12.5\"

:: ��ȡ�ű�����·��
set script_path=%~dp0
:: ����ű�����Ŀ¼,��Ϊ���Ӱ��ű���ִ�еĳ���Ĺ���Ŀ¼
set old_cd=%cd%
cd /d %~dp0

:: ������������
set debug_mode="false"
set cpu_mode=x86

echo=
echo=
echo ---------------------------------------------------------------
echo ���������[debug/release x86/x64]
echo ---------------------------------------------------------------

:: ���������� /i���Դ�Сд
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

:: ��ʾ
echo ��ǰ����ģʽΪdebug %debug_mode% %cpu_mode%

:: ������������
if /i %cpu_mode% == x86 (
    set qt_msvc_path=%qt_msvc_path%msvc2017\bin
) else (
    set qt_msvc_path=%qt_msvc_path%msvc2017_64\bin
)

set build_path=%script_path%build
set PATH=%qt_msvc_path%;%PATH%

:: ע��vc����
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
echo ��ʼqmake����
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
echo ��ɣ�
echo ---------------------------------------------------------------

:return
cd %old_cd%