QT += core gui widgets network
CONFIG += c++17

TEMPLATE = app

TARGET = client

OBJECTS_DIR = $$PWD/obj
MOC_DIR = $$PWD/moc
RCC_DIR = $$PWD/rcc
UI_DIR = $$PWD/ui

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    registerdialog.cpp \
    logindialog.cpp \
    chatwindow.cpp

HEADERS += \
    mainwindow.h \
    registerdialog.h \
    logindialog.h \
    chatwindow.h

FORMS += \
    mainwindow.ui \
    registerdialog.ui \
    logindialog.ui \
    chatwindow.ui

DESTDIR = $$PWD/../bin
