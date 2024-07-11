#include "colorcell.h"
#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>

ColorCell::ColorCell(int row, int col, const QColor &color,
                     QGraphicsItem *parent)
    : QGraphicsRectItem(parent), row(row), col(col), clicked_state(false),
      plotted_state(false), is_recording_video(false), hover_color(0, 255, 0),
      selected_color(0, 128, 0), plotted_color(239, 35, 60), selected_width(2),
      plotted_width(4) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
  setBrush(QBrush(color));
  setAcceptHoverEvents(true);
  setPen(Qt::NoPen);

  selected_tooltip = new QLabel();
  selected_tooltip->setWindowFlags(Qt::ToolTip);
  selected_tooltip->hide();

  hover_tooltip = new QLabel();
  hover_tooltip->setWindowFlags(Qt::ToolTip);
  hover_tooltip->hide();

  connect(&tooltip_timer, &QTimer::timeout, this, &ColorCell::showHoverTooltip);
  tooltip_timer.setSingleShot(true);
  tooltip_timer.setInterval(250);
}

void ColorCell::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *widget) {
  QGraphicsRectItem::paint(painter, option, widget);

  if (!text.isEmpty()) {
    painter->save();
    QFont font = painter->font();
    font.setPointSize(10);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(Qt::black);
    painter->drawText(boundingRect(), Qt::AlignCenter, text);
    painter->restore();
  }

  if (!is_recording_video) {
    if (clicked_state) {
      painter->setPen(QPen(selected_color, selected_width));
      painter->drawRect(rect().adjusted(selected_width / 2, selected_width / 2,
                                        -selected_width / 2,
                                        -selected_width / 2));
    } else if (plotted_shape == "") {
      painter->setPen(QPen(plotted_color, selected_width));
      painter->drawEllipse(rect().adjusted(plotted_width / 2, plotted_width / 2,
                                           -plotted_width / 2,
                                           -plotted_width / 2));
    } else if (plotted_shape == "󰔷") {
      painter->setPen(QPen(plotted_color, selected_width));
      QPolygonF triangle;
      triangle << QPointF(rect().center().x(), rect().top() + plotted_width / 2)
               << QPointF(rect().left() + plotted_width / 2,
                          rect().bottom() - plotted_width / 2)
               << QPointF(rect().right() - plotted_width / 2,
                          rect().bottom() - plotted_width / 2);
      painter->drawPolygon(triangle);
    } else if (plotted_shape == "x") {
      painter->setPen(QPen(plotted_color, selected_width));
      painter->drawLine(
          rect().topLeft() + QPointF(plotted_width / 2, plotted_width / 2),
          rect().bottomRight() - QPointF(plotted_width / 2, plotted_width / 2));
      painter->drawLine(
          rect().topRight() - QPointF(plotted_width / 2, -plotted_width / 2),
          rect().bottomLeft() + QPointF(plotted_width / 2, -plotted_width / 2));
    } else if (plotted_shape == "") {
      painter->setPen(QPen(plotted_color, selected_width));
      painter->drawRect(rect().adjusted(plotted_width / 2, plotted_width / 2,
                                        -plotted_width / 2,
                                        -plotted_width / 2));
    }
  }
}

void ColorCell::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
  if (!is_recording_video && !clicked_state) {
    tooltip_timer.start();
  }
  QGraphicsRectItem::hoverEnterEvent(event);
}

void ColorCell::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  if (!is_recording_video && !clicked_state) {
    tooltip_timer.stop();
    hover_tooltip->hide();
  }
  QGraphicsRectItem::hoverLeaveEvent(event);
}

void ColorCell::showHoverTooltip() {
  if (!clicked_state && isUnderMouse()) {
    hover_tooltip->setText(QString("(%1, %2)").arg(row + 1).arg(col + 1));
    QPoint tooltip_pos = QCursor::pos() + QPoint(20, -20);
    hover_tooltip->move(tooltip_pos);
    hover_tooltip->show();
  }
}

void ColorCell::showSelectedTooltip() {
  selected_tooltip->setText(QString("(%1, %2)").arg(row + 1).arg(col + 1));
  QPoint tooltip_pos = QCursor::pos() + QPoint(20, -20);
  selected_tooltip->move(tooltip_pos);
  selected_tooltip->show();
}

void ColorCell::setColor(const QColor &color, qreal strength, qreal opacity) {
  strength = qBound(0.0, strength, 1.0);
  QColor hsv_color = color.toHsv();
  hsv_color.setHsv(hsv_color.hue(), int(hsv_color.saturation() * strength),
                   hsv_color.value());
  QColor rgb_color = hsv_color.toRgb();
  rgb_color.setAlphaF(opacity);
  setBrush(QBrush(rgb_color));
}

void ColorCell::setText(const QString &text) {
  this->text = text;
  update();
}

void ColorCell::hideSelectedTooltip() { selected_tooltip->hide(); }

void ColorCell::hideHoverTooltip() { hover_tooltip->hide(); }
