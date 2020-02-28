echo
echo
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

# 从环境变量获取必要参数
# 例如 /home/barry/Qt5.9.6/5.9.6/gcc_64
echo ENV_QT_GCC $ENV_QT_GCC

# 获取绝对路径，保证其他目录执行此脚本依然正确
{
cd $(dirname "$0")
script_path=$(pwd)
cd -
} &> /dev/null # disable output
# 设置当前目录，cd的目录影响接下来执行程序的工作目录
old_cd=$(pwd)
cd $(dirname "$0")

# 启动参数声明
publish_dir=$1

# 提示
echo current publish dir: $publish_dir

# 环境变量设置
keymap_path=$script_path/../../keymap
# config_path=$script_path/../../config

publish_path=$script_path/$publish_dir
release_path=$script_path/../../output/linux/release

export PATH=$ENV_QT_GCC/bin:$PATH

if [ -d "$publish_path" ]; then
    rm -rf $publish_path
fi

# 复制要发布的包
cp -r $release_path $publish_path
cp -r $keymap_path $publish_path/QtScrcpy.app/Contents/MacOS
# cp -r $config_path $publish_path/QtScrcpy.app/Contents/MacOS

# 添加qt依赖包
macdeployqt $publish_path/QtScrcpy.app

# 删除多余qt依赖包

# PlugIns
rm -rf $publish_path/QtScrcpy.app/Contents/PlugIns/iconengines
# 截图功能需要libqjpeg.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqgif.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqicns.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqico.dylib
# rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqjpeg.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqmacheif.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqmacjp2.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqtga.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqtiff.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqwbmp.dylib
rm -f $publish_path/QtScrcpy.app/Contents/PlugIns/imageformats/libqwebp.dylib
rm -rf $publish_path/QtScrcpy.app/Contents/PlugIns/virtualkeyboard
rm -rf $publish_path/QtScrcpy.app/Contents/PlugIns/printsupport
rm -rf $publish_path/QtScrcpy.app/Contents/PlugIns/platforminputcontexts
rm -rf $publish_path/QtScrcpy.app/Contents/PlugIns/iconengines
rm -rf $publish_path/QtScrcpy.app/Contents/PlugIns/bearer

# Frameworks
rm -rf $publish_path/QtScrcpy.app/Contents/Frameworks/QtVirtualKeyboard.framework
rm -rf $publish_path/Contents/Frameworks/QtSvg.framework

# qml
rm -rf $publish_path/QtScrcpy.app/Contents/Frameworks/QtQml.framework
rm -rf $publish_path/QtScrcpy.app/Contents/Frameworks/QtQuick.framework

echo
echo
echo ---------------------------------------------------------------
echo finish!!!
echo ---------------------------------------------------------------

# 恢复当前目录
cd $old_cd
exit 0
