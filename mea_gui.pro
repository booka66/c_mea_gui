greaterThan(QT_MAJOR_VERSION, 4): QT += core gui widgets printsupport
CONFIG += c++17
SOURCES += main.cpp \
           mainwindow.cpp \
           gridwidget.cpp \
           colorcell.cpp \
           qcustomplot.cpp \
           graphwidget.cpp
HEADERS += mainwindow.h \
           gridwidget.h \
           colorcell.h \
           qcustomplot.h \
           graphwidget.h
