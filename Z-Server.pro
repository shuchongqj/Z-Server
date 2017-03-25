QT += core gui widgets

CONFIG += c++11

TARGET = Z-Server

CONFIG += console
win32 {
CONFIG += no_batch
}

unix {
LIBS += -L../Urho3D/lib -lX11 -lXtst -lUrho3D -ldl
QMAKE_CXX = /usr/bin/gcc
QMAKE_CXXFLAGS += -pthread -fno-strict-aliasing -Wno-sign-compare -Wno-unused-parameter
}

win32 {
LIBS += "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\User32.Lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\Shell32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\Ws2_32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\kernel32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\gdi32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\winspool.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\ole32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\oleaut32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\uuid.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\comdlg32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\advapi32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\winmm.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\imm32.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\version.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\dbghelp.lib" \
        "C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x64\d3dcompiler.lib" \
        "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64\d3d9.lib" \
        ..\Z-Server\pthread\lib\pthreadVC2.lib \
        ..\Urho3D\lib\Urho3D.lib
}

TEMPLATE = app


DEFINES += WIN32 _WINDOWS NDEBUG _SECURE_SCL=0 URHO3D_MINIDUMPS URHO3D_FILEWATCHER URHO3D_PROFILING URHO3D_LOGGING URHO3D_THREADING URHO3D_ANGELSCRIPT URHO3D_NAVIGATION URHO3D_PHYSICS URHO3D_URHO2D URHO3D_CXX11 _CRT_SECURE_NO_WARNINGS HAVE_STDINT_H
INCLUDEPATH += ../Urho3D/include
INCLUDEPATH += ../Urho3D/include/Urho3D/ThirdParty

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
