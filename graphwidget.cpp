#include "graphwidget.h"
#include <algorithm>

GraphWidget::GraphWidget(QWidget *parent)
    : QWidget(parent), activePlotIndex(0), doShowRegions(true), doShowMiniMap(true), lastActivePlotIndex(-1)
{
    layout = new QVBoxLayout(this);
    setupMinimap();
    setupPlotWidgets();

    xData.resize(4);
    yData.resize(4);
}

void GraphWidget::setupMinimap()
{
    minimap = new QCustomPlot(this);
    minimap->setFixedHeight(100);
    minimap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    minimap->xAxis->setVisible(false);
    minimap->yAxis->setVisible(false);
    minimap->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    minimapRegion = new QCPItemRect(minimap);
    minimapRegion->setPen(QPen(Qt::blue));
    minimapRegion->setBrush(QBrush(QColor(0, 0, 255, 50)));
    minimapRegion->topLeft->setType(QCPItemPosition::ptPlotCoords);
    minimapRegion->bottomRight->setType(QCPItemPosition::ptPlotCoords);

    connect(minimap->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(updatePlotViews()));

    layout->addWidget(minimap);
}

void GraphWidget::setupPlotWidgets()
{
    plotsContainer = new QWidget(this);
    plotsLayout = new QVBoxLayout(plotsContainer);
    plotsLayout->setContentsMargins(0, 0, 0, 0);
    plotsLayout->setSpacing(0);

    for (int i = 0; i < 4; ++i) {
        QCustomPlot* plotWidget = new QCustomPlot(this);
        plotWidget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        plotWidget->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
        plotWidget->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);

        QCPItemLine* redLine = new QCPItemLine(plotWidget);
        redLine->setPen(QPen(Qt::red, 2));
        redLines.append(redLine);

        QCPGraph* plot = plotWidget->addGraph();
        plots.append(plot);

        plotWidgets.append(plotWidget);
        plotsLayout->addWidget(plotWidget);

        connect(plotWidget->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(updateMinimap()));
    }

    layout->addWidget(plotsContainer);
}

void GraphWidget::plot(const QVector<double>& x, const QVector<double>& y, const QString& title,
                       const QString& xlabel, const QString& ylabel, int plotIndex,
                       const QString& /* shape */, const QVector<QVector<double>>& seizures,
                       const QVector<QVector<double>>& se)
{
    xData[plotIndex] = x;
    yData[plotIndex] = y;

    auto regions = getRegions(seizures, se);
    const auto& seizureRegions = regions.first;
    const auto& seRegions = regions.second;

    QVector<double> downsampleY = downsampleData(x, y, 1000);  // Assuming GRAPH_DOWNSAMPLE is 1000
    QVector<double> downsampleX(downsampleY.size());
    for (int i = 0; i < downsampleX.size(); ++i) {
        downsampleX[i] = x[i * (x.size() - 1) / (downsampleX.size() - 1)];
    }

    plots[plotIndex]->setData(downsampleX, downsampleY);

    QCustomPlot* plot = plotWidgets[plotIndex];
    plot->xAxis->setLabel(xlabel);
    plot->yAxis->setLabel(ylabel);

    // Set title
    QCPTextElement *titleElement = new QCPTextElement(plot);
    titleElement->setText(title);
    titleElement->setFont(QFont("sans", 12, QFont::Bold));
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0, titleElement);

    // Add regions
    for (const auto& region : seRegions) {
        QCPItemRect* rect = new QCPItemRect(plot);
        rect->topLeft->setCoords(region.at(0), plot->yAxis->range().upper);
        rect->bottomRight->setCoords(region.at(1), plot->yAxis->range().lower);
        rect->setBrush(QBrush(QColor(255, 183, 3, 128)));
        rect->setPen(Qt::NoPen);
    }

    for (const auto& region : seizureRegions) {
        QCPItemRect* rect = new QCPItemRect(plot);
        rect->topLeft->setCoords(region.at(0), plot->yAxis->range().upper);
        rect->bottomRight->setCoords(region.at(1), plot->yAxis->range().lower);
        rect->setBrush(QBrush(QColor(0, 150, 199, 128)));
        rect->setPen(Qt::NoPen);
    }

    plot->replot();

    if (plotIndex == 0) {
        updateMinimap();
    }
}

void GraphWidget::updateMinimap()
{
    if (!doShowMiniMap) return;

    const QVector<double>& activeXData = xData[activePlotIndex];
    const QVector<double>& activeYData = yData[activePlotIndex];

    if (activeXData.isEmpty() || activeYData.isEmpty()) return;

    if (activePlotIndex != lastActivePlotIndex) {
        QVector<double> downsampleY = downsampleData(activeXData, activeYData, 500);
        QVector<double> downsampleX(downsampleY.size());
        for (int i = 0; i < downsampleX.size(); ++i) {
            downsampleX[i] = activeXData[i * (activeXData.size() - 1) / (downsampleX.size() - 1)];
        }
        minimap->graph(0)->setData(downsampleX, downsampleY);
        lastActivePlotIndex = activePlotIndex;
    }

    QCustomPlot* activePlot = plotWidgets[activePlotIndex];
    QCPRange xRange = activePlot->xAxis->range();

    minimapRegion->topLeft->setCoords(xRange.lower, minimap->yAxis->range().upper);
    minimapRegion->bottomRight->setCoords(xRange.upper, minimap->yAxis->range().lower);

    minimap->replot();
}

void GraphWidget::updatePlotViews()
{
    QCPRange range = minimap->xAxis->range();
    for (QCustomPlot* plot : plotWidgets) {
        plot->xAxis->setRange(range);
        plot->replot();
    }
}

void GraphWidget::toggleRegions()
{
    doShowRegions = !doShowRegions;
    for (QCustomPlot* plot : plotWidgets) {
        // Iterate through all items in the plot
        for (int i = 0; i < plot->itemCount(); ++i) {
            QCPAbstractItem* item = plot->item(i);
            if (QCPItemRect* rect = qobject_cast<QCPItemRect*>(item)) {
                rect->setVisible(doShowRegions);
            }
        }
        plot->replot();
    }
}

void GraphWidget::toggleRedLines()
{
    for (QCPItemLine* line : redLines) {
        line->setVisible(!line->visible());
    }
    for (QCustomPlot* plot : plotWidgets) {
        plot->replot();
    }
}

void GraphWidget::toggleMiniMap(bool checked)
{
    doShowMiniMap = checked;
    minimap->setVisible(checked);
    if (checked) {
        updateMinimap();
    }
}

void GraphWidget::updateRedLines(double value, double samplingRate)
{
    double position = value / samplingRate;
    for (QCPItemLine* line : redLines) {
        line->start->setCoords(position, 0);
        line->end->setCoords(position, 1);
    }
    for (QCustomPlot* plot : plotWidgets) {
        plot->replot();
    }
}

void GraphWidget::changeViewMode(const QString& mode)
{
    QCP::Interactions interactions = QCP::iRangeDrag | QCP::iRangeZoom;
    if (mode == "pan") {
        interactions |= QCP::iSelectPlottables;
    }
    for (QCustomPlot* plot : plotWidgets) {
        plot->setInteractions(interactions);
    }
}

std::pair<QVector<QVector<double>>, QVector<QVector<double>>> GraphWidget::getRegions(const QVector<QVector<double>>& seizures,
                                                 const QVector<QVector<double>>& se)
{
    QVector<QVector<double>> seizureRegions, seRegions;

    for (const auto& timerange : se) {
        seRegions.append({timerange.at(0), timerange.at(1)});
    }

    for (const auto& timerange : seizures) {
        double start = timerange.at(0), stop = timerange.at(1);
        bool overlaps = false;
        for (const auto& seRegion : seRegions) {
            if ((start <= seRegion.at(0) && seRegion.at(0) <= stop) ||
                (seRegion.at(0) <= start && start <= seRegion.at(1))) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            seizureRegions.append({start, stop});
        }
    }

    return std::make_pair(seizureRegions, seRegions);
}

QVector<double> GraphWidget::downsampleData(const QVector<double>& x, const QVector<double>& y, int numPoints)
{
    if (x.size() <= numPoints) {
        return y;
    }

    QVector<double> downsampleY(numPoints);
    double ratio = static_cast<double>(x.size() - 1) / (numPoints - 1);

    for (int i = 0; i < numPoints; ++i) {
        int lowIndex = static_cast<int>(i * ratio);
        int highIndex = std::min(static_cast<int>((i + 1) * ratio), static_cast<int>(x.size()) - 1);

        QVector<double>::const_iterator begin = y.begin() + lowIndex;
        QVector<double>::const_iterator end = y.begin() + highIndex + 1;

        downsampleY[i] = *std::min_element(begin, end);
    }

    return downsampleY;
}
