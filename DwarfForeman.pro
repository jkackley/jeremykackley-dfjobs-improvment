#-------------------------------------------------
#
# Project created by QtCreator 2010-07-25T06:45:57
#
#-------------------------------------------------

QT       += core gui xml

TARGET = "DwarfForeman"
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    statusframe.cpp \
    configframe.cpp \
    formanthread.cpp \
    logframe.cpp \
    treethread.cpp

HEADERS  += mainwindow.h \
    statusframe.h \
    configframe.h \
    dwarfforeman.h \
    formanthread.h \
    logframe.h \
    treethread.h

FORMS    += mainwindow.ui \
    statusframe.ui \
    configframe.ui \
    logframe.ui

QMAKE_CXXFLAGS += -march=pentium4 -O2 -Wall

OTHER_FILES += dfjobs.xml
