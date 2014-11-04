#-------------------------------------------------
#
# Project created by QtCreator 2014-11-04T14:00:07
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = libeventHttpServerTcpUdp
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

LIBS += -levent

SOURCES += main.cpp \
    libeventserver.cpp

HEADERS += \
    libeventserver.h
