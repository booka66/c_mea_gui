greaterThan(QT_MAJOR_VERSION, 4): QT += core gui widgets printsupport
INCLUDEPATH += /opt/homebrew/Cellar/hdf5/1.14.3_1/include
LIBS += -L/opt/homebrew/Cellar/hdf5/1.14.3_1/lib -lhdf5 -lhdf5_cpp
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
