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
    uistatemanager.cpp \
    videodecoderthread.cpp

HEADERS += \
    adbprocess.h \
    androidkeycodes.h \
    controlsender.h \
    devicemanager.h \
    devicewindow.h \
    mainwindow.h \
    scrcpyoptions.h \
    uistatemanager.h \
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
FFMPEG_PATH = $$PWD/3rdparty/ffmpeg
INCLUDEPATH += $$FFMPEG_PATH/include
LIBS += -L$$FFMPEG_PATH/lib \
        -lavformat \
        -lavcodec \
        -lavutil \
        -lswscale \
        -lswresample

RESOURCES += \
    resources.qrc

win32 {
    RC_FILE = app.rc
}
