最后同步scrcpy b91ecf52256da73f5c8dca04fb82c13ec826cbd7

# TODO
## 低优先级
0. 中文输入（server需要改为apk，作为一个输入法，暂不实现）（或者有其他方式案件注入方式，例如搜狗手机输入法可以监听当前注入？）
1. 鼠标事件相关系列 b35733edb6df2a00b6af9b1c98627d344c377963
2. [跳过帧改为动态配置，而不是静态编译](https://github.com/Genymobile/scrcpy/commit/ebccb9f6cc111e8acfbe10d656cac5c1f1b744a0)
3. [单独线程统计帧率](https://github.com/Genymobile/scrcpy/commit/e2a272bf99ecf48fcb050177113f903b3fb323c4)
4. ui提供show touch设置
5. 隐藏手机皮肤开关

## 中优先级
0. [截屏保存为jpg](https://blog.csdn.net/m0_37684310/article/details/77950390)
1. 版本号升级优化
2. linux打包以及版本号
3. 自动打包脚本
4. 按键映射可配置
5. 脚本
6. 群控
7. 配置文件
8. 软硬解配置，去皮肤配置
9. 窗口可改变大小
10. 竖屏全屏不拉伸画面
11. 分辨率码率可自定义

# BUG
1. 魅族手机提示cant open video stream，解决方法 https://dim.red/2019/03/03/scrcpy_usage/

# mark
[ffmpeg编译参数详解](https://www.cnblogs.com/wainiwann/p/4204230.html)
