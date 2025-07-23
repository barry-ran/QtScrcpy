# 声明ts文件
set(QC_TS_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/zh_CN.ts 
    ${CMAKE_CURRENT_SOURCE_DIR}/en_US.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/ja_JP.ts
)
# 设置qm文件生成目录
set_source_files_properties(${QC_TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")
# 引入LinguistTools
find_package(QT NAMES Qt6 Qt5 COMPONENTS LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LinguistTools REQUIRED)

# qt5_create_translation会依次执行 lupdate更新ts、lrelease更新qm
qt5_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../.. ${QC_TS_FILES})
# 自定义目标依赖QM_FILES，否则不会生成qm文件
add_custom_target(QC_QM_GENERATOR DEPENDS ${QM_FILES})

# qt5_create_translation的bug：cmake clean的时候会删除翻译好的ts文件，导致翻译丢失
# （qt官方说qt6没问题，只用qt6的可以考虑qt5_create_translation）
# 网上查到的CLEAN_NO_CUSTOM办法只能在makefile生成器下生效，解决不了问题
# https://cmake.org/cmake/help/latest/prop_dir/CLEAN_NO_CUSTOM.html
# set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)
# 目前唯一的解决办法是每次clean后，都手动在git中恢复一下ts文件

#[[
总结:
cmake qt项目下，利用cmake脚本有三种方式处理翻译：
1. 完全使用qt自带的cmake LinguistTools脚本：qt5_create_translation&qt5_add_translation
这两个脚本都满足不了需求：
qt5_add_translation只能根据已有ts文件生成qm文件（lrelease），不能更新ts文件(lupdate)
qt5_create_translation在cmake clean的时候会删除翻译好的ts文件，导致翻译丢失

2. cmake add_custom_command + cmake LinguistTools脚本（其实qt5_create_translation内部使用的也是add_custom_command）
例如add_custom_command执行lupdate，配合qt5_add_translation更新qm，
参考：https://github.com/maratnek/QtFirstProgrammCMake/blob/2c93b59e2ba85ff6ee0e727487e14003381687d3/CMakeLists.txt

3. 完全使用cmake命令来执行lupdate和lrelease
例如add_custom_command/add_custom_target/execute_process都可以实现执行lupdate和lrelease命令

上面3个方案都有一个共同问题：就是翻译文件处理都是和编译绑定在一起的，每次编译都会检测执行，实际的翻译工作是所有
编程工作都完成以后，统一执行一次lupdate、翻译、lrelease就可以了，不应该和编译绑定在一起
所以写两个shell脚本lupdate.sh和lrelease.sh来处理比较合适，其实非常简单：
1. 更新ts：lupdate -no-obsolete ./QtScrcpy -ts ./QtScrcpy/res/i18n/en_US.ts ./QtScrcpy/res/i18n/zh_CN.ts
2. 手动翻译ts
3. 发布：lrelease ./QtScrcpy/res/i18n/en_US.ts ./QtScrcpy/res/i18n/zh_CN.ts

参考文档
1. qt知道qt5_create_translation的bug，但是不肯解决，只确定了qt6没问题 https://bugreports.qt.io/browse/QTBUG-96549
2. https://doc.qt.io/qt-5/qtlinguist-cmake-qt5-add-translation.html
3. https://doc.qt.io/qt-5/qtlinguist-cmake-qt5-create-translation.html
4. execute_process 参考：https://blog.csdn.net/u010255072/article/details/120326833
5. add_custom_target 参考：https://www.cnblogs.com/apocelipes/p/14355460.html
