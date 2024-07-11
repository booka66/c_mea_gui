#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>
#include <qcustomplot.h>

class GraphWidget : public QWidget {
  Q_OBJECT

public:
  explicit GraphWidget(QWidget *parent = nullptr);
  void plot(const QVector<double> &x, const QVector<double> &y,
            const QString &title, const QString &xlabel, const QString &ylabel,
            int plotIndex, const QString &shape,
            const QVector<QVector<double>> &seizures,
            const QVector<QVector<double>> &se);
  void toggleRegions();
  void toggleRedLines();
  void toggleMiniMap(bool checked);
  void updateRedLines(double value, double samplingRate);
  void changeViewMode(const QString &mode);
  void simplePlot(const QList<double> &x, const QList<double> &y,
                  int graphIndex);
  QVector<QCustomPlot *> plotWidgets;

signals:
  void regionClicked(double start, double stop, int plotIndex);
  void saveSinglePlot();
  void saveAllPlots();
  void rightClickedAndDragged(QCPRange range, int plotIndex);

private slots:
  void updateMinimap();
  void updatePlotViews();
  void onMousePress(QMouseEvent *event);
  void onMouseMove(QMouseEvent *event);
  void onMouseRelease(QMouseEvent *event);

private:
  QVBoxLayout *layout;
  QCustomPlot *minimap;
  QCPItemRect *minimapRegion;
  QWidget *plotsContainer;
  QVBoxLayout *plotsLayout;
  QVector<QCPItemLine *> redLines;
  QVector<QCPGraph *> plots;
  QVector<QVector<double>> xData;
  QVector<QVector<double>> yData;
  int activePlotIndex;
  bool doShowRegions;
  bool doShowMiniMap;
  int lastActivePlotIndex;

  bool isRightClickDragging;
  bool isLeftClickDragging;
  QPoint dragStartPosition;
  int currentDraggingPlotIndex;

  void setupMinimap();
  void setupPlotWidgets();
  void redrawRegions(double start, double stop,
                     const QVector<bool> &plottedChannels);
  std::pair<QVector<QVector<double>>, QVector<QVector<double>>>
  getRegions(const QVector<QVector<double>> &seizures,
             const QVector<QVector<double>> &se);
  void showRegions();
  void hideRegions();
  QVector<double> downsampleData(const QVector<double> &x,
                                 const QVector<double> &y, int numPoints);
  void setupPlotInteractions();
  void linkAxes();
};

#endif // GRAPHWIDGET_H
