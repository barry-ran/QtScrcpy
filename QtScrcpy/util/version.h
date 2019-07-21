#ifndef VERSION_H
#define VERSION_H

#define PRODUCT_ICON           "QtScrcpy.ico" // 图标

#define QTSCRCPY_MAJOR 1
#define QTSCRCPY_MINOR 0
#define QTSCRCPY_PATCH 2

#define QTSCRCPY_VERSION_CHK(major, minor, patch) \
    (((major)<<16) | ((minor)<<8) | (patch))

// 数值形式版本号，用于版本升级判断
#define QTSCRCPY_VERSION QTSCRCPY_VERSION_CHK(QTSCRCPY_MAJOR, QTSCRCPY_MINOR, QTSCRCPY_PATCH)

#define _TOSTR(x)   #x
#define TOSTR(x)  _TOSTR(x)

// 字符串形式版本号，用于版本显示
/* the following are compile time version */
/* C++11 requires a space between literal and identifier */
#define QTSCRCPY_VERSION_STR        TOSTR(QTSCRCPY_MAJOR) "." TOSTR(QTSCRCPY_MINOR) "." TOSTR(QTSCRCPY_PATCH)

// rc形式版本号，用于windos rc文件中指定版本号
#define QTSCRCPY_VERSION_RES QTSCRCPY_MAJOR,QTSCRCPY_MINOR,QTSCRCPY_PATCH


// windos rc相关定义
#define FILE_VERSION           QTSCRCPY_VERSION_RES   // 文件版本
#define FILE_VERSION_STR       QTSCRCPY_VERSION_STR
#define PRODUCT_VERSION        QTSCRCPY_VERSION_RES   // 产品版本
#define PRODUCT_VERSION_STR    QTSCRCPY_VERSION_STR
#define COMPANY_NAME           "RanKun"
#define FILE_DESCRIPTION       "android real time projection software"  // 文件说明
#define LEGAL_COPYRIGHT        "Copyright (C) RanKun 2018-2028. All rights reserved." // 版权
#define PRODUCT_NAME           "QtScrcpy"        // 产品名称

#endif // VERSION_H
