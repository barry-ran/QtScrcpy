
echo
echo
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

# 从环境变量获取必要参数
# 例如 /Users/barry/Qt5.12.5/5.12.5
echo ENV_QT_PATH $ENV_QT_PATH
qt_clang_path=$ENV_QT_PATH/clang_64

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
build_mode=debug

echo
echo
echo ---------------------------------------------------------------
echo check build param[debug/release]
echo ---------------------------------------------------------------

# 编译参数检查
build_mode=$(echo $1 | tr '[:upper:]' '[:lower:]')
if [[ $build_mode != "release" && $build_mode != "debug" ]]; then
    echo "error: unkonow build mode -- $1"
    exit 1
fi

# 提示
echo current build mode: $build_mode

# 环境变量设置
export PATH=$qt_clang_path/bin:$PATH

echo
echo
echo ---------------------------------------------------------------
echo begin qmake build
echo ---------------------------------------------------------------

# 删除输出目录
output_path=$script_path../../output/mac/$build_mode
if [ -d "$output_path" ]; then
    rm -rf $output_path
fi
# 删除临时目录
temp_path=$script_path/../temp
if [ -d "$temp_path" ]; then
    rm -rf $temp_path
fi
mkdir $temp_path
cd $temp_path

qmake_params="-spec macx-clang"
if [ $build_mode == "debug" ]; then
    qmake_params="$qmake_params CONFIG+=debug CONFIG+=x86_64 CONFIG+=qml_debug"
else
    qmake_params="$qmake_params CONFIG+=x86_64 CONFIG+=qtquickcompiler"
fi

# qmake ../../all.pro -spec macx-clang CONFIG+=debug CONFIG+=x86_64 CONFIG+=qml_debug
qmake ../../all.pro $qmake_params
if [ $? -ne 0 ] ;then
    echo "qmake failed"
    exit 1
fi

make -j8
if [ $? -ne 0 ] ;then
    echo "make failed"
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
