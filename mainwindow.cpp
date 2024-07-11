#include "mainwindow.h"
#include "graphwidget.h"
#include "gridwidget.h"
#include <H5Cpp.h>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <stdexcept>
#include <string>
#include <vector>

#include <H5Cpp.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

struct HDFData {
  std::vector<double> signal;
  std::vector<double> time;
  double samplingRate;
  hsize_t totalFrames;
};

HDFData loadHDFData(const std::string &filePath) {
  try {
    H5::H5File file(filePath, H5F_ACC_RDONLY);

    // Read NRecFrames
    H5::DataSet nRecFramesDataset =
        file.openDataSet("/3BRecInfo/3BRecVars/NRecFrames");
    hsize_t nRecFrames;
    nRecFramesDataset.read(&nRecFrames, H5::PredType::NATIVE_HSIZE);
    std::cout << "NRecFrames: " << nRecFrames << std::endl;

    // Read sampling rate
    H5::DataSet sampRateDataset =
        file.openDataSet("/3BRecInfo/3BRecVars/SamplingRate");
    double sampRate;
    sampRateDataset.read(&sampRate, H5::PredType::NATIVE_DOUBLE);
    std::cout << "Sampling Rate: " << sampRate << std::endl;

    // Read raw data
    H5::DataSet rawDataset = file.openDataSet("/3BData/Raw");
    H5::DataSpace dataspace = rawDataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();
    hsize_t dims[2];
    dataspace.getSimpleExtentDims(dims, NULL);
    std::cout << "Raw data dimensions: " << dims[0] << " x " << dims[1]
              << std::endl;

    // Determine the number of frames to read
    hsize_t framesToRead = std::min(nRecFrames, dims[0] * dims[1]);
    std::cout << "Frames to read: " << framesToRead << std::endl;

    // Prepare to read data
    std::vector<int> rawData(framesToRead);
    H5::DataSpace memspace(1, &framesToRead);

    // Select hyperslab
    hsize_t start[2] = {0, 0};
    hsize_t count[2] = {1, 1};
    hsize_t stride[2] = {1, 1};
    hsize_t block[2] = {1, 1};

    if (rank == 2) {
      count[0] = std::min(framesToRead, dims[0]);
      count[1] = 1;
      block[1] = std::min(framesToRead / count[0], dims[1]);
    } else if (rank == 1) {
      count[0] = framesToRead;
    } else {
      throw std::runtime_error("Unexpected dataset rank: " +
                               std::to_string(rank));
    }

    std::cout << "Hyperslab selection: start=" << start[0] << "," << start[1]
              << " count=" << count[0] << "," << count[1]
              << " stride=" << stride[0] << "," << stride[1]
              << " block=" << block[0] << "," << block[1] << std::endl;

    dataspace.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

    // Read the data
    rawDataset.read(rawData.data(), H5::PredType::NATIVE_INT, memspace,
                    dataspace);

    // Read conversion factors
    int signalInversion = 1;
    double maxVolt = 1.0, minVolt = -1.0;
    int bitDepth = 16;

    try {
      H5::DataSet signalInversionDataset =
          file.openDataSet("/3BRecInfo/3BRecVars/SignalInversion");
      signalInversionDataset.read(&signalInversion, H5::PredType::NATIVE_INT);
    } catch (H5::Exception &e) {
      std::cerr
          << "Warning: Couldn't read SignalInversion, using default value."
          << std::endl;
    }

    try {
      H5::DataSet maxVoltDataset =
          file.openDataSet("/3BRecInfo/3BRecVars/MaxVolt");
      maxVoltDataset.read(&maxVolt, H5::PredType::NATIVE_DOUBLE);
    } catch (H5::Exception &e) {
      std::cerr << "Warning: Couldn't read MaxVolt, using default value."
                << std::endl;
    }

    try {
      H5::DataSet minVoltDataset =
          file.openDataSet("/3BRecInfo/3BRecVars/MinVolt");
      minVoltDataset.read(&minVolt, H5::PredType::NATIVE_DOUBLE);
    } catch (H5::Exception &e) {
      std::cerr << "Warning: Couldn't read MinVolt, using default value."
                << std::endl;
    }

    try {
      H5::DataSet bitDepthDataset =
          file.openDataSet("/3BRecInfo/3BRecVars/BitDepth");
      bitDepthDataset.read(&bitDepth, H5::PredType::NATIVE_INT);
    } catch (H5::Exception &e) {
      std::cerr << "Warning: Couldn't read BitDepth, using default value."
                << std::endl;
    }

    double qLevel = std::pow(2, bitDepth);
    double fromQLevelToUVolt = (maxVolt - minVolt) / qLevel;
    double ADCCountsToMV = signalInversion * fromQLevelToUVolt;
    double MVOffset = signalInversion * minVolt;

    // Process data
    std::vector<double> signal(framesToRead);
    for (hsize_t i = 0; i < framesToRead; ++i) {
      signal[i] = ((rawData[i] * ADCCountsToMV) + MVOffset) / 1000000.0;
    }

    // Generate time vector
    std::vector<double> time(framesToRead);
    for (hsize_t i = 0; i < framesToRead; ++i) {
      time[i] = static_cast<double>(i) / sampRate;
    }

    std::cout << "Data processing completed successfully" << std::endl;

    return {signal, time, sampRate, framesToRead};
  } catch (H5::Exception &error) {
    std::ostringstream oss;
    oss << "Error reading HDF5 file: " << error.getDetailMsg() << "\n";
    throw std::runtime_error(oss.str());
  } catch (std::exception &e) {
    throw std::runtime_error("Error in loadHDFData: " + std::string(e.what()));
  }
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Spatial SE Viewer");

  createCentralWidget();
  createRightPane();
  createBottomPane();

  createMenuBar();

  setCentralWidget(mainTabWidget);
}

void MainWindow::testGraph() {
  std::string filePath =
      "~/Desktop/Neo Se/6-13-2024-slice2b_00_resample_300.brw";

  // Expand the ~ in the file path
  if (filePath.find("~") == 0) {
    filePath.replace(0, 1, QDir::homePath().toStdString());
  }

  // Check if file exists
  QFileInfo checkFile(QString::fromStdString(filePath));
  if (!checkFile.exists() || !checkFile.isFile()) {
    QMessageBox::critical(this, "Error",
                          "File does not exist: " +
                              QString::fromStdString(filePath));
    return;
  }

  // Check file permissions
  if (!checkFile.isReadable()) {
    QMessageBox::critical(this, "Error",
                          "File is not readable: " +
                              QString::fromStdString(filePath));
    return;
  }

  try {
    // Try to open the file without reading data
    H5::H5File file(filePath, H5F_ACC_RDONLY);

    // If we got here, the file opened successfully
    QMessageBox::information(this, "Success", "HDF5 file opened successfully.");

    // Now try to load the data
    HDFData data = loadHDFData(filePath);

    // If we got here, data was loaded successfully
    QMessageBox::information(
        this, "Success",
        QString(
            "Data loaded successfully.\nTotal frames: %1\nSampling rate: %2")
            .arg(data.totalFrames)
            .arg(data.samplingRate));

    // Plot the data
    for (int i = 0; i < graphWidget->plotWidgets.size(); ++i) {
      graphWidget->simplePlot(
          QList<double>(data.time.begin(), data.time.end()),
          QList<double>(data.signal.begin(), data.signal.end()), i);
    }
  } catch (const H5::FileIException &e) {
    QMessageBox::critical(
        this, "HDF5 File Error",
        QString("Failed to open HDF5 file: %1\nError details: %2")
            .arg(QString::fromStdString(filePath))
            .arg(e.getCDetailMsg()));
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Error",
                          QString("Failed to load data: %1").arg(e.what()));
  }
}

void stressTest(GridWidget *gridWidget) { gridWidget->startAnimation(); }

void MainWindow::createMenuBar() {
  QMenuBar *menuBar = new QMenuBar(this);
  setMenuBar(menuBar);

  QMenu *fileMenu = menuBar->addMenu("File");
  fileMenu->addAction("Open file");
  fileMenu->addAction("Save MEA as video");
  fileMenu->addAction("Save MEA as png");
  fileMenu->addAction("Save channel plots");
  fileMenu->addAction("Save MEA with channel plots");
  fileMenu->addSeparator();
  QAction *stressTestAction = new QAction("Stress Test");
  stressTestAction->setShortcut(QKeySequence("Ctrl+T"));
  stressTestAction->connect(stressTestAction, &QAction::triggered,
                            [this]() { stressTest(gridWidget); });
  fileMenu->addAction(stressTestAction);

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

  QPushButton *editRasterSettingsButton =
      new QPushButton("Edit Raster Settings");
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
  // Connect runButton to a slot that runs the analysis
  connect(runButton, &QPushButton::clicked, this, &MainWindow::testGraph);
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

  progressBar = new QSlider(
      Qt::Horizontal); // Replace with EEGScrubberWidget when implemented
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
  speedCombo->addItems(
      {"0.01", "0.1", "0.25", "0.5", "1.0", "2.0", "4.0", "16.0"});
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
