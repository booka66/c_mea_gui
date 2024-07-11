#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gridwidget.h"
#include "graphwidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QTimer>
#include <qcustomplot.h>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    void createMenuBar();
    void createCentralWidget();
    void createRightPane();
    void createBottomPane();

    QTabWidget *mainTabWidget;
    QTabWidget *tabWidget;
    QWidget *rightPane;
    QWidget *bottomPane;
    QSlider *opacitySlider;
    QCheckBox *showOrderCheckbox;
    QComboBox *orderCombo;
    QPushButton *openButton;
    QCheckBox *lowRamCheckbox;
    QPushButton *viewButton;
    QPushButton *runButton;
    QPushButton *clearButton;
    QSlider *progressBar;  // Replace with EEGScrubberWidget when implemented
    QPushButton *skipBackwardButton;
    QPushButton *prevFrameButton;
    QPushButton *playPauseButton;
    QPushButton *nextFrameButton;
    QPushButton *skipForwardButton;
    QComboBox *speedCombo;
    QTimer *playbackTimer;
    GridWidget *gridWidget;
    GraphWidget *graphWidget;
    QCustomPlot *secondPlotWidget;
};

#endif // MAINWINDOW_H
