echo
echo
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

# 从环境变量获取必要参数
# 例如 /Users/barry/Qt5.12.5/5.12.5
echo ENV_QT_PATH $ENV_QT_PATH

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
cpu_arch=$2

echo
echo
echo ---------------------------------------------------------------
echo check cpu arch[x64/arm64]
echo ---------------------------------------------------------------

if [[ $cpu_arch != "x64" && $cpu_arch != "arm64" ]]; then
    echo "error: unkonow cpu mode -- $2"
    exit 1
fi

# 提示
echo current cpu mode: $cpu_arch

if [ $cpu_arch == "x64" ]; then
    qt_clang_path=$ENV_QT_PATH/clang_64
else
    qt_clang_path=$ENV_QT_PATH/macos
fi

# 提示
echo current publish dir: $publish_dir

# 环境变量设置
keymap_path=$script_path/../../keymap
# config_path=$script_path/../../config

publish_path=$script_path/$publish_dir
release_path=$script_path/../../output/$cpu_arch/RelWithDebInfo

export PATH=$qt_clang_path/bin:$PATH

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

# Create an ad-hoc signature for every executable component after macdeployqt
# and the size-reduction cleanup are finished. This makes the final bundle
# internally consistent without requiring an Apple Developer certificate.
# It does not replace Developer ID signing/notarization, so Gatekeeper may
# still require the user to explicitly allow an Internet-downloaded app.
echo "ad-hoc code signing macOS app bundle"
app_path=$publish_path/QtScrcpy.app
frameworks_path=$app_path/Contents/Frameworks

# Remove links targeting plugins deleted above; a dangling link prevents
# codesign --strict from sealing the outer bundle.
while IFS= read -r -d '' link; do
    if [ ! -e "$link" ]; then
        rm -f "$link"
    fi
done < <(find "$app_path" -type l -print0)

# Sign individual Mach-O files first, including adb and Qt plugin dylibs.
# Then seal framework bundles and finally the outer application bundle.
while IFS= read -r -d '' binary; do
    if file -b "$binary" | grep -q '^Mach-O'; then
        codesign --force --sign - --timestamp=none "$binary"
    fi
done < <(find "$app_path/Contents" -type f -print0)

if [ -d "$frameworks_path" ]; then
    while IFS= read -r -d '' framework; do
        codesign --force --sign - --timestamp=none --deep "$framework"
    done < <(find "$frameworks_path" -type d -name '*.framework' -print0)
fi

codesign --force --sign - --timestamp=none --deep "$app_path"
if ! codesign --verify --deep --strict --verbose=2 "$app_path"; then
    echo "ad-hoc code signature verification failed"
    cd $old_cd
    exit 1
fi

echo
echo
echo ---------------------------------------------------------------
echo finish!!!
echo ---------------------------------------------------------------

# 恢复当前目录
cd $old_cd
exit 0
