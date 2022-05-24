#pragma once
#include <QString>

namespace qsc {

struct DeviceParams {
    QString serverLocalPath = "";     // 本地安卓server路径
    QString serverRemotePath = "";    // 要推送到远端设备的server路径
    QString recordPath = "";          // 视频保存路径
    QString recordFileFormat = "mp4"; // 视频保存格式 mp4/mkv
    QString serial = "";              // 设备序列号
    quint16 localPort = 27183;        // reverse时本地监听端口
    quint16 maxSize = 720;            // 视频分辨率
    quint32 bitRate = 2000000;        // 视频比特率
    quint32 maxFps = 60;              // 视频最大帧率
    bool recordFile = false;          // 录制到文件
    bool closeScreen = false;         // 启动时自动息屏
    bool useReverse = true;           // true:先使用adb reverse，失败后自动使用adb forward；false:直接使用adb forward
    bool display = true;              // 是否显示画面（或者仅仅后台录制）
    QString gameScript = "";          // 游戏映射脚本
    bool renderExpiredFrames = false; // 是否渲染延迟视频帧
    int lockVideoOrientation = -1;    // 是否锁定视频方向
    bool stayAwake = false;           // 是否保持唤醒
};
    
}
