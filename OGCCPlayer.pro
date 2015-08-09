#-------------------------------------------------
#
# Project created by QtCreator 2014-11-17T15:06:02
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OGCCPlayer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    treeitem.cpp \
    mywidgets.cpp \
    models/mytreemodel.cpp \
    models/mylistmodel.cpp \
    models/mydirmodel.cpp \
    search/searchdialog.cpp

HEADERS  += mainwindow.h \
    treeitem.h \
    mywidgets.h \
    models/mytreemodel.h \
    models/mylistmodel.h \
    models/mydirmodel.h \
    search/searchdialog.h

FORMS    += mainwindow.ui \
    search/searchdialog.ui

CONFIG += c++11
CONFIG += qwt

RESOURCES += \
    res.qrc

#LIBS += -bass

INCLUDEPATH += E:/Dropbox/MyApps/QtCreator/build-OGCCPlayer-Desktop_Qt_5_4_2_MinGW_32bit-Debug/bass24/c
DEPENDPATH += E:/Dropbox/MyApps/QtCreator/build-OGCCPlayer-Desktop_Qt_5_4_2_MinGW_32bit-Debug/bass24/c

LIBS += -LE:/Dropbox/MyApps/QtCreator/build-OGCCPlayer-Desktop_Qt_5_4_2_MinGW_32bit-Debug/bass24 -lbass
