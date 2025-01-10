
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
build_mode=RelWithDebInfo
cpu_arch=arm64

echo
echo
echo ---------------------------------------------------------------
echo check build param[Debug/Release/MinSizeRel/RelWithDebInfo]
echo ---------------------------------------------------------------

# 编译参数检查
build_mode=$(echo $1)
if [[ $build_mode != "Release" && $build_mode != "Debug" && $build_mode != "MinSizeRel" && $build_mode != "RelWithDebInfo" ]]; then
    echo "error: unkonow build mode -- $1"
    exit 1
fi

echo
echo
echo ---------------------------------------------------------------
echo check cpu arch[x64/arm64]
echo ---------------------------------------------------------------

cpu_arch=$(echo $2)
if [[ $cpu_arch != "x64" && $cpu_arch != "arm64" ]]; then
    echo "error: unkonow cpu mode -- $2"
    exit 1
fi

# 提示
echo current build mode: $build_mode
echo current cpu mode: $cpu_arch

cmake_arch=x86_64
if [ $cpu_arch == "x64" ]; then
    qt_cmake_path=$ENV_QT_PATH/clang_64/lib/cmake/Qt5
    cmake_arch=x86_64
else
    qt_cmake_path=$ENV_QT_PATH/macos/lib/cmake/Qt6
    cmake_arch=arm64
fi

echo
echo
echo ---------------------------------------------------------------
echo begin cmake build
echo ---------------------------------------------------------------

# 删除输出目录
output_path=$script_path../../output
if [ -d "$output_path" ]; then
    rm -rf $output_path
fi
# 删除编译目录
build_path=$script_path/../build_temp
if [ -d "$build_path" ]; then
    rm -rf $build_path
fi
mkdir $build_path
cd $build_path

cmake_params="-DCMAKE_PREFIX_PATH=$qt_cmake_path -DCMAKE_BUILD_TYPE=$build_mode -DCMAKE_OSX_ARCHITECTURES=$cmake_arch"
cmake $cmake_params ../..
if [ $? -ne 0 ] ;then
    echo "cmake failed"
    exit 1
fi

cmake --build . --config $build_mode -j8
if [ $? -ne 0 ] ;then
    echo "cmake build failed"
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
