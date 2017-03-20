QT += core gui widgets

CONFIG += c++11

TARGET = Z-Server

CONFIG += console
win32 {
CONFIG += no_batch
}

unix {
LIBS += -ldl -lX11 -lXtst
QMAKE_CXX = /usr/bin/gcc
}

win32 {
LIBS += "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\User32.Lib" \
    "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\Shell32.lib" \
    "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\Ws2_32.lib" \
    ..\Z-Server\pthread\lib\pthreadVC2.lib
}

TEMPLATE = app

SOURCES += main.cpp \
    TinyXML2/tinyxml2.cpp \
    dlfcn-win32/dlfcn.c \
    parser-ext.cpp \
    Server/proto-parser.cpp \
    Server/server.cpp \
    Server/net-hub.cpp \
    mainwindow.cpp \
    main-hub.cpp \
    Dialogs/change_level_dialog.cpp

HEADERS += \
    dirent-win32/dirent.h \
    dlfcn-win32/dlfcn.h \
    TinyXML2/tinyxml2.h \
    logger.h \
    pthread/include/pthread.h \
    pthread/include/sched.h \
    pthread/include/semaphore.h \
    parser-ext.h \
    Server/proto-parser.h \
    Server/proto-util.h \
    Server/protocol.h \
    Server/server.h \
    Server/net-hub.h \
    mainwindow.h \
    main-hub.h \
    Dialogs/change_level_dialog.h

DISTFILES += \
    pthread/lib/pthreadVC2.lib \
    pthread/dll/pthreadVC2.dll

FORMS += \
    mainwindow.ui \
    Dialogs/change_level_dialog.ui
