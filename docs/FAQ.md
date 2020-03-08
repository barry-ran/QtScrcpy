# Frequently Asked Questions
一些经常问的问题

如果在此文档没有解决你的问题，描述你的问题，截图软件控制台中打印的日志，一起发到QQ群里提问。

## 支持声音（软件不做支持）
[关于转发安卓声音到PC的讨论](https://github.com/Genymobile/scrcpy/issues/14#issuecomment-543204526)

## 无法输入中文
手机端安装搜狗输入法/QQ输入法就可以支持输入中文了

## 可以看到画面，但无法控制
有些手机(小米等手机)需要额外打开控制权限，检查是否USB调试里打开了允许模拟点击

![image](image/USB调试(安全设置).jpg)

## 手机通过数据线连接电脑，刷新设备列表以后，没有任何设备出现
随便下载一个手机助手，尝试连接成功以后，再用QtScrcpy刷新设备列表连接

## 错误信息：AdbProcess::error:adb server version (40) doesnt match this client (41)
任务管理找到adb进程并杀死，重新操作即可

## 错误信息：Could not open video stream
导致这个错误的原因有很多，最简单的解决方法是在分辨率设置中，选择一个较低的分辨率

