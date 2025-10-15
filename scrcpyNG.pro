QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    adbprocess.cpp \
    controlsender.cpp \
    devicemanager.cpp \
    devicewindow.cpp \
    main.cpp \
    mainwindow.cpp \
    scrcpyoptions.cpp \
    videodecoderthread.cpp

HEADERS += \
    adbprocess.h \
    controlsender.h \
    devicemanager.h \
    devicewindow.h \
    mainwindow.h \
    scrcpyoptions.h \
    videodecoderthread.h

FORMS += \
    devicewindow.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


# ===================================================================
# FFmpeg Integration
# ===================================================================
# 设置 FFmpeg 开发库的根目录
# $$PWD 表示当前 .pro 文件所在的目录
FFMPEG_PATH = $$PWD/3rdparty/ffmpeg
# 1. 添加头文件搜索路径
INCLUDEPATH += $$FFMPEG_PATH/include
# 2. 添加库文件搜索路径和要链接的库
# -L 告诉链接器去哪里找库文件
# -l 指定具体要链接哪个库 (去掉前缀 lib 和后缀 .a 或 .lib)
# 这个配置适用于 MinGW (g++) 编译器
LIBS += -L$$FFMPEG_PATH/lib \
        -lavformat \
        -lavcodec \
        -lavutil \
        -lswscale \
        -lswresample
