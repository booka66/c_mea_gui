#include "mainwindow.h"
#include "graphwidget.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMenuBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Spatial SE Viewer");

    createMenuBar();
    createCentralWidget();
    createRightPane();
    createBottomPane();

    setCentralWidget(mainTabWidget);
}

void MainWindow::createMenuBar() {
  QMenuBar *menuBar = new QMenuBar(this);
  setMenuBar(menuBar);

  QMenu *fileMenu = menuBar->addMenu("File");
  fileMenu->addAction("Open file");
  fileMenu->addAction("Save MEA as video");
  fileMenu->addAction("Save MEA as png");
  fileMenu->addAction("Save channel plots");
  fileMenu->addAction("Save MEA with channel plots");

  QMenu *editMenu = menuBar->addMenu("Edit");
  editMenu->addAction("Set Low Pass Filter");
  editMenu->addAction("Set raster downsample factor");
  editMenu->addAction("Create raster");
  editMenu->addSeparator();

  QMenu *viewMenu = menuBar->addMenu("View");
  viewMenu->addAction("Legend")->setCheckable(true);
  viewMenu->addAction("Mini-map")->setCheckable(true);
  viewMenu->addAction("Playheads")->setCheckable(true);
  viewMenu->addAction("Anti-aliasing")->setCheckable(true);
  viewMenu->addAction("Spread lines")->setCheckable(true);
  viewMenu->addAction("Propagation lines")->setCheckable(true);
  viewMenu->addAction("Detected events")->setCheckable(true);
  viewMenu->addAction("False color map")->setCheckable(true);
  viewMenu->addSeparator();
  viewMenu->addAction("Seizure regions")->setCheckable(true);
  viewMenu->addAction("Spectrograms")->setCheckable(true);
  viewMenu->addSeparator();
  viewMenu->addAction("Set bin size");
  viewMenu->addAction("Set order amount");
}

void MainWindow::createCentralWidget() {
    mainTabWidget = new QTabWidget(this);

    QWidget *mainTab = new QWidget();
    QHBoxLayout *mainTabLayout = new QHBoxLayout(mainTab);

    // Left pane
    QWidget *leftPane = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPane);

    tabWidget = new QTabWidget();
    leftLayout->addWidget(tabWidget);

    // MEA Grid tab
    QWidget *meaGridWidget = new QWidget();
    QVBoxLayout *meaGridLayout = new QVBoxLayout(meaGridWidget);

    gridWidget = new GridWidget(64, 64);
    gridWidget->setMinimumHeight(gridWidget->height() + 100);

    QWidget *squareWidget = new QWidget();
    QVBoxLayout *squareLayout = new QVBoxLayout(squareWidget);
    squareLayout->addWidget(gridWidget);

    QGraphicsView *legendView = new QGraphicsView();
    QGraphicsScene *legendScene = new QGraphicsScene();
    legendView->setScene(legendScene);
    legendView->setFixedHeight(100);

    QWidget *legendWidget = new QWidget();
    legendWidget->setVisible(false);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(legendWidget);
    topLayout->addWidget(squareWidget);

    meaGridLayout->addLayout(topLayout);
    meaGridLayout->addWidget(legendView);

    tabWidget->addTab(meaGridWidget, "MEA Grid");

    // Raster Plot tab
    QWidget *rasterPlotWidget = new QWidget();
    QVBoxLayout *rasterPlotLayout = new QVBoxLayout(rasterPlotWidget);

    secondPlotWidget = new QCustomPlot();
    rasterPlotLayout->addWidget(secondPlotWidget);

    QHBoxLayout *rasterSettingsLayout = new QHBoxLayout();
    rasterSettingsLayout->addWidget(new QLabel("Raster Settings:"));

    QPushButton *editRasterSettingsButton = new QPushButton("Edit Raster Settings");
    rasterSettingsLayout->addWidget(editRasterSettingsButton);

    QPushButton *createGroupsButton = new QPushButton("Create Groups");
    rasterSettingsLayout->addWidget(createGroupsButton);

    QPushButton *toggleColorModeButton = new QPushButton("Toggle Color Mode");
    rasterSettingsLayout->addWidget(toggleColorModeButton);

    rasterPlotLayout->addLayout(rasterSettingsLayout);

    tabWidget->addTab(rasterPlotWidget, "Raster Plot");

    mainTabLayout->addWidget(leftPane);

    // Stats tab
    QWidget *statsTab = new QWidget();

    mainTabWidget->addTab(mainTab, "Main");
    mainTabWidget->addTab(statsTab, "Stats");
}

void MainWindow::createRightPane() {
    rightPane = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Create a QSplitter to allow resizing
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    rightLayout->addWidget(splitter);

    // Graph pane
    QWidget *graphPane = new QWidget();
    QVBoxLayout *graphLayout = new QVBoxLayout(graphPane);
    graphLayout->setContentsMargins(0, 0, 0, 0);
    graphWidget = new GraphWidget(this);
    graphLayout->addWidget(graphWidget);
    splitter->addWidget(graphPane);

    // Settings pane
    QWidget *settingsPane = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPane);
    QHBoxLayout *settingsTopLayout = new QHBoxLayout();
    settingsLayout->addLayout(settingsTopLayout);
    settingsTopLayout->addWidget(new QLabel("Image Opacity:"));
    opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setRange(0, 100);
    opacitySlider->setValue(100);
    opacitySlider->setTickPosition(QSlider::TicksBelow);
    opacitySlider->setTickInterval(25);
    settingsTopLayout->addWidget(opacitySlider);
    showOrderCheckbox = new QCheckBox("Show Order");
    showOrderCheckbox->setEnabled(false);
    settingsTopLayout->addWidget(showOrderCheckbox);
    orderCombo = new QComboBox();
    orderCombo->addItems({"Default", "Order by Seizure", "Order by SE"});
    settingsTopLayout->addWidget(orderCombo);
    QHBoxLayout *controlLayout = new QHBoxLayout();
    settingsLayout->addLayout(controlLayout);
    openButton = new QPushButton(" Open File");
    controlLayout->addWidget(openButton);
    lowRamCheckbox = new QCheckBox(" Low RAM Mode");
    controlLayout->addWidget(lowRamCheckbox);
    viewButton = new QPushButton(" Quick View");
    controlLayout->addWidget(viewButton);
    runButton = new QPushButton(" Run Analysis");
    controlLayout->addWidget(runButton);
    clearButton = new QPushButton(" Clear Plots");
    controlLayout->addWidget(clearButton);
    splitter->addWidget(settingsPane);

    // Set initial sizes for the splitter
    splitter->setSizes({900, 100}); // Adjust these values as needed

    // Add right pane to main tab layout
    QWidget *mainTab = mainTabWidget->widget(0);
    QHBoxLayout *mainTabLayout = qobject_cast<QHBoxLayout *>(mainTab->layout());
    if (mainTabLayout) {
        mainTabLayout->addWidget(rightPane);
    }
}

void MainWindow::createBottomPane() {
    bottomPane = new QWidget();
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomPane);

    QHBoxLayout *playbackLayout = new QHBoxLayout();
    bottomLayout->addLayout(playbackLayout);

    progressBar = new QSlider(Qt::Horizontal);  // Replace with EEGScrubberWidget when implemented
    playbackLayout->addWidget(progressBar, 1);

    skipBackwardButton = new QPushButton("");
    playbackLayout->addWidget(skipBackwardButton);

    prevFrameButton = new QPushButton("");
    playbackLayout->addWidget(prevFrameButton);

    playPauseButton = new QPushButton("");
    playbackLayout->addWidget(playPauseButton);

    nextFrameButton = new QPushButton("");
    playbackLayout->addWidget(nextFrameButton);

    skipForwardButton = new QPushButton("");
    playbackLayout->addWidget(skipForwardButton);

    speedCombo = new QComboBox();
    speedCombo->addItems({"0.01", "0.1", "0.25", "0.5", "1.0", "2.0", "4.0", "16.0"});
    speedCombo->setCurrentIndex(2);
    speedCombo->view()->setMinimumWidth(100);
    playbackLayout->addWidget(speedCombo);

    playbackTimer = new QTimer(this);

    // Add bottom pane to right pane layout
    QWidget *mainTab = mainTabWidget->widget(0);
    QHBoxLayout *mainTabLayout = qobject_cast<QHBoxLayout *>(mainTab->layout());
    if (mainTabLayout) {
        QWidget *rightPane = mainTabLayout->itemAt(1)->widget();
        QVBoxLayout *rightLayout = qobject_cast<QVBoxLayout *>(rightPane->layout());
        if (rightLayout) {
            rightLayout->addWidget(bottomPane);
        }
    }
}
