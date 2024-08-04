QT += core network
QT -= gui

CONFIG += c++17

OBJECTS_DIR = $$PWD/obj
MOC_DIR = $$PWD/moc
RCC_DIR = $$PWD/rcc

SOURCES += main.cpp \
           server.cpp

HEADERS += server.h

DESTDIR = $$PWD/../bin
