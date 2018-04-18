
QT       += core gui

TARGET =  CANBootloader
TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


//DEFINES += LANGUE_EN

RESOURCES += \
    ../source/image.qrc
RC_FILE = \
    ../source/ico.rc

FORMS += \
    ../source/mainwindow_ch.ui \
    ../source/scandevrangedialog.ui

OTHER_FILES += \
    ../source/ico.rc

HEADERS += \
    ../source/mainwindow.h \
    ../source/scandevrangedialog.h \
    ../source/ControlCAN.h \
    ../source/can_driver.h

SOURCES += \
    ../source/main.cpp \
    ../source/mainwindow.cpp \
    ../source/scandevrangedialog.cpp

unix|win32: LIBS += -L$$PWD/lib/win32/ -lControlCAN

INCLUDEPATH += $$PWD/lib/win32
DEPENDPATH += $$PWD/lib/win32
