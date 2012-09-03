#-------------------------------------------------
#
# Project created by QtCreator 2011-09-30T13:30:13
#
#-------------------------------------------------

QT       += core gui

TARGET = simpleMediaPlayer
TEMPLATE = app


SOURCES += main.cpp\
        mediaplayer.cpp \
    lrcwindow.cpp

HEADERS  += mediaplayer.h \
    lrcwindow.h

FORMS    += mediaplayer.ui

RESOURCES += \
    images.qrc

QT   += phonon

RC_FILE = icon.rc

OTHER_FILES += \
    icon.rc
