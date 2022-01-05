
echo
echo
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

# 从环境变量获取必要参数
# 例如 /home/barry/Qt5.9.6/5.9.6
echo ENV_QT_PATH $ENV_QT_PATH
qt_cmake_path=$ENV_QT_PATH/gcc_64/lib/cmake/Qt5

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

# 提示
echo current build mode: $build_mode

# 环境变量设置
#export PATH=$qt_gcc_path/bin:$PATH

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
# 删除临时目录
build_path=$script_path/../build_temp
if [ -d "$build_path" ]; then
    rm -rf $build_path
fi
mkdir $build_path
cd $build_path

cmake_params="-DCMAKE_PREFIX_PATH=$qt_cmake_path -DCMAKE_BUILD_TYPE=$build_mode"
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
