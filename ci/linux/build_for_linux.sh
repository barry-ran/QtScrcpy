echo ---------------------------------------------------------------
echo Check \& Set Environment Variables
echo ---------------------------------------------------------------

# Get Qt path
# ENV_QT_PATH example: /home/barry/Qt5.9.6/5.9.6
echo Current ENV_QT_PATH: $ENV_QT_PATH
echo Current directory: $(pwd)
# Set variables
qt_cmake_path=$ENV_QT_PATH/gcc_64/lib/cmake/Qt5
qt_gcc_path=$ENV_QT_PATH/gcc_64
export PATH=$qt_gcc_path/bin:$PATH

# Remember working directory
old_cd=$(pwd)

# Set working dir to the script's path (go up two levels from ci/linux/ to project root)
cd $(dirname "$0")/../../

echo
echo
echo ---------------------------------------------------------------
echo Check Build Parameters
echo ---------------------------------------------------------------
echo Possible build modes: Debug/Release/MinSizeRel/RelWithDebInfo

build_mode="$1"
if [[ $build_mode != "Release" && $build_mode != "Debug" && $build_mode != "MinSizeRel" && $build_mode != "RelWithDebInfo" ]]; then
    echo "error: unknown build mode, exiting......"
    exit 1
fi

echo Current build mode: $build_mode

echo
echo
echo ---------------------------------------------------------------
echo CMake Build Begins
echo ---------------------------------------------------------------

# Remove output folder
output_path=./output
if [ -d "$output_path" ]; then
    rm -rf $output_path
fi

cmake_params="-DCMAKE_PREFIX_PATH=$qt_cmake_path -DCMAKE_BUILD_TYPE=$build_mode"
cmake $cmake_params .
if [ $? -ne 0 ] ;then
    echo "error: CMake failed, exiting......"
    exit 1
fi

cmake --build . --config "$build_mode" -j8
if [ $? -ne 0 ] ;then
    echo "error: CMake build failed, exiting......"
    exit 1
fi

echo
echo
echo ---------------------------------------------------------------
echo CMake Build Succeeded
echo ---------------------------------------------------------------

# Resume current directory
cd $old_cd
exit 0
