#include "mainwindow.h"
#include "constants.h"
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
#include <utility>
#include <vector>

#include <H5Cpp.h>
#include <iostream>
#include <string>
#include <vector>

struct ChannelData {
  std::vector<double> signal;
  std::vector<int> name;
};

struct ElectrodeInfo {
  int Row;
  int Col;
};

std::pair<std::vector<int>, std::vector<int>>
getChs(const std::string &FilePath) {
  try {
    H5::H5File file(FilePath, H5F_ACC_RDONLY);

    H5::DataSet dataset = file.openDataSet("/3BRecInfo/3BMeaStreams/Raw/Chs");

    H5::DataSpace dataspace = dataset.getSpace();
    hsize_t dims[2];
    int ndims = dataspace.getSimpleExtentDims(dims, NULL);

    H5::CompType mtype(sizeof(int) * 2);
    mtype.insertMember("Row", 0, H5::PredType::NATIVE_INT);
    mtype.insertMember("Col", sizeof(int), H5::PredType::NATIVE_INT);

    std::vector<std::pair<int, int>> data(dims[0]);

    dataset.read(data.data(), mtype);

    std::vector<int> rows(dims[0]);
    std::vector<int> cols(dims[0]);

    for (size_t i = 0; i < dims[0]; ++i) {
      rows[i] = data[i].first;
      cols[i] = data[i].second;
    }

    return std::make_pair(std::move(rows), std::move(cols));
  } catch (H5::Exception &error) {
    std::cerr << "H5 Exception: ";
    error.printErrorStack();
    throw std::runtime_error("Error reading HDF5 file");
  } catch (std::exception &e) {
    std::cerr << "Standard exception: " << e.what() << std::endl;
    throw;
  } catch (...) {
    std::cerr << "Unknown exception occurred" << std::endl;
    throw;
  }
}

std::vector<ChannelData> get_cat_envelop(const std::string &FileName) {
  try {
    H5::H5File file(FileName, H5F_ACC_RDONLY);

    auto readDataset = [&file](const std::string &path) {
      H5::DataSet dataset = file.openDataSet(path);
      H5::DataSpace dataspace = dataset.getSpace();
      H5T_class_t type_class = dataset.getTypeClass();

      if (type_class == H5T_INTEGER) {
        int data;
        dataset.read(&data, H5::PredType::NATIVE_INT);
        return static_cast<double>(data);
      } else if (type_class == H5T_FLOAT) {
        double data;
        dataset.read(&data, H5::PredType::NATIVE_DOUBLE);
        return data;
      } else {
        throw std::runtime_error("Unsupported data type");
      }
    };

    long long NRecFrames =
        static_cast<long long>(readDataset("/3BRecInfo/3BRecVars/NRecFrames"));
    double sampRate = readDataset("/3BRecInfo/3BRecVars/SamplingRate");
    double signalInversion =
        readDataset("/3BRecInfo/3BRecVars/SignalInversion");
    double maxUVolt = readDataset("/3BRecInfo/3BRecVars/MaxVolt");
    double minUVolt = readDataset("/3BRecInfo/3BRecVars/MinVolt");
    int bitDepth =
        static_cast<int>(readDataset("/3BRecInfo/3BRecVars/BitDepth"));

    uint64_t qLevel =
        static_cast<uint64_t>(1) ^ static_cast<uint64_t>(bitDepth);

    double fromQLevelToUVolt =
        (maxUVolt - minUVolt) / static_cast<double>(qLevel);
    double ADCCountsToMV = signalInversion * fromQLevelToUVolt;
    double MVOffset = signalInversion * minUVolt;

    auto [Rows, Cols] = getChs(FileName);
    int total_channels = Rows.size();

    H5::DataSet full_data = file.openDataSet("/3BData/Raw");
    H5::DataSpace dataspace = full_data.getSpace();
    int rank = dataspace.getSimpleExtentNdims();
    std::vector<hsize_t> dims(rank);
    dataspace.getSimpleExtentDims(dims.data(), NULL);

    // QString debugInfo = QString("Raw data dimensions: %1\n"
    //                             "NRecFrames: %2\n"
    //                             "Sampling Rate: %3\n"
    //                             "Signal Inversion: %4\n"
    //                             "Max Voltage: %5\n"
    //                             "Min Voltage: %6\n"
    //                             "Bit Depth: %7\n"
    //                             "Q Level: %8\n"
    //                             "From Q Level to Voltage: %9\n"
    //                             "ADC Counts to MV: %10\n"
    //                             "MV Offset: %11\n"
    //                             "Total channels: %12\n"
    //                             "Calculated channels: %13")
    //                         .arg(dims[0])
    //                         .arg(NRecFrames)
    //                         .arg(sampRate)
    //                         .arg(signalInversion)
    //                         .arg(maxUVolt)
    //                         .arg(minUVolt)
    //                         .arg(bitDepth)
    //                         .arg(qLevel)
    //                         .arg(fromQLevelToUVolt)
    //                         .arg(ADCCountsToMV)
    //                         .arg(MVOffset)
    //                         .arg(total_channels)
    //                         .arg(dims[0] / NRecFrames);
    //
    // QMessageBox::information(nullptr, "Debug Info", debugInfo);

    if (rank != 1) {
      throw std::runtime_error("Unexpected number of dimensions in raw data");
    }

    if (dims[0] != static_cast<hsize_t>(NRecFrames * total_channels)) {
      QString warningMsg = QString("Warning: Data size mismatch.\n"
                                   "Expected size: %1\n"
                                   "Actual size: %2")
                               .arg(NRecFrames * total_channels)
                               .arg(dims[0]);
      QMessageBox::warning(nullptr, "Size Mismatch", warningMsg);
    }

    std::vector<ChannelData> channelDataList;
    channelDataList.reserve(total_channels);

    std::vector<int16_t> all_data(dims[0]);
    full_data.read(all_data.data(), H5::PredType::NATIVE_INT16);

    for (int k = 0; k < total_channels; ++k) {
      ChannelData ch_data;
      ch_data.signal.reserve(NRecFrames);

      for (long long i = 0; i < NRecFrames; ++i) {
        double val = static_cast<double>(all_data[i * total_channels + k]);
        val = (val * ADCCountsToMV + MVOffset) / 1000000.0; // Convert to mV
        ch_data.signal.push_back(val);
      }
      double mean =
          std::accumulate(ch_data.signal.begin(), ch_data.signal.end(), 0.0) /
          ch_data.signal.size();

      for (auto &val : ch_data.signal) {
        val -= mean;
      }

      ch_data.name = {Rows[k], Cols[k]};
      channelDataList.push_back(std::move(ch_data));
    }

    return channelDataList;
  } catch (H5::Exception &error) {
    std::string errorMsg = "H5 Exception: ";
    error.printErrorStack();
    errorMsg += error.getDetailMsg();
    QMessageBox::critical(nullptr, "HDF5 Error",
                          QString::fromStdString(errorMsg));
    throw std::runtime_error("Error reading HDF5 file");
  } catch (std::exception &e) {
    QMessageBox::critical(nullptr, "Standard Exception", e.what());
    throw;
  } catch (...) {
    QMessageBox::critical(nullptr, "Unknown Exception",
                          "An unknown exception occurred");
    throw;
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
      "~/Jake-Squared/Sz_SE_Detection/5_13_24_slice1B_resample_100.brw";

  if (filePath.find("~") == 0) {
    filePath.replace(0, 1, QDir::homePath().toStdString());
  }

  QFileInfo checkFile(QString::fromStdString(filePath));
  if (!checkFile.exists() || !checkFile.isFile()) {
    QMessageBox::critical(this, "Error",
                          "File does not exist: " +
                              QString::fromStdString(filePath));
    return;
  }

  if (!checkFile.isReadable()) {
    QMessageBox::critical(this, "Error",
                          "File is not readable: " +
                              QString::fromStdString(filePath));
    return;
  }

  try {
    H5::H5File file(filePath, H5F_ACC_RDONLY);

    auto channelDataList = get_cat_envelop(filePath);

    // Plot the data for each channel
    for (size_t i = 0; i < PLOT_COUNT; ++i) {
      const auto &channelData = channelDataList[i];

      // Create x-axis data (time)
      QVector<double> xData(channelData.signal.size());
      for (size_t j = 0; j < xData.size(); ++j) {
        xData[j] = static_cast<double>(j);
      }

      // Convert signal data to QVector
      QVector<double> yData(channelData.signal.begin(),
                            channelData.signal.end());

      // Plot the data
      graphWidget->simplePlot(xData, yData, i);
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
