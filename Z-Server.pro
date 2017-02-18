QT -= gui

CONFIG += c++11

TARGET = Z-Server
CONFIG += console
win32 {
CONFIG += no_batch
}
CONFIG -= app_bundle

unix {
LIBS += -ldl -lX11 -lXtst
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
    parserext.cpp \
    dlfcn-win32/dlfcn.c \
    protoparser.cpp \
    hub.cpp \
    server.cpp

HEADERS += \
    hub.h \
    dirent-win32/dirent.h \
    dlfcn-win32/dlfcn.h \
    TinyXML2/tinyxml2.h \
    logger.h \
    parserext.h \
    pthread/include/pthread.h \
    pthread/include/sched.h \
    pthread/include/semaphore.h \
    protoparser.h \
    protocol.h \
    server.h

DISTFILES += \
    pthread/lib/pthreadVC2.lib \
    pthread/dll/pthreadVC2.dll
