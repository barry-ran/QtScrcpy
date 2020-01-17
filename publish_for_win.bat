@echo off
set qt_msvc_path="D:\Qt\Qt5.12.4\5.12.4\"

:: ��ȡ�ű�����·��
set script_path=%~dp0
:: ����ű�����Ŀ¼,��Ϊ���Ӱ��ű���ִ�еĳ���Ĺ���Ŀ¼
set old_cd=%cd%
cd /d %~dp0

:: ������������
set cpu_mode=x86
if /i "%1"=="x86" (
    set cpu_mode=x86
)
if /i "%1"=="x64" (
    set cpu_mode=x64
)

:: ������������

set adb_path=%script_path%third_party\adb\win\*.*
set jar_path=%script_path%third_party\scrcpy-server
set keymap_path=%script_path%keymap
set config_path=%script_path%config

if /i %cpu_mode% == x86 (
    set publish_path=%script_path%QtScrcpy-win32\
    set release_path=%script_path%output\win\x86\release
    set qt_msvc_path=%qt_msvc_path%msvc2017\bin
) else (
    set publish_path=%script_path%QtScrcpy-win64\
    set release_path=%script_path%output\win\x64\release
    set qt_msvc_path=%qt_msvc_path%msvc2017_64\bin
)
set PATH=%qt_msvc_path%;%PATH%

if exist %publish_path% (
    rmdir /s/q %publish_path%
)

:: ����Ҫ�����İ�
xcopy %release_path% %publish_path% /E /Y
xcopy %adb_path% %publish_path% /Y
xcopy %jar_path% %publish_path% /Y
xcopy %keymap_path% %publish_path%keymap\ /E /Y
xcopy %config_path% %publish_path%config\ /E /Y

:: ���qt������
windeployqt %publish_path%\QtScrcpy.exe

:: ɾ������qt������
rmdir /s/q %publish_path%\iconengines
rmdir /s/q %publish_path%\imageformats
rmdir /s/q %publish_path%\translations
if /i %cpu_mode% == x86 (
    del %publish_path%\vc_redist.x86.exe
) else (
    del %publish_path%\vc_redist.x64.exe
)

echo=
echo=
echo ---------------------------------------------------------------
echo ��ɣ�
echo ---------------------------------------------------------------

:return
cd %old_cd%