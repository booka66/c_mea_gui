#include "gridwidget.h"
#include <QAction>
#include <QContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPixmap>
#include <QResizeEvent>
#include <cmath>

GridWidget::GridWidget(int rows, int cols, QWidget *parent)
    : QGraphicsView(parent), rows(rows), cols(cols), is_recording_video(false),
      selected_channel(nullptr), animation_phase(0), opacity(1.0) {
  scene = new QGraphicsScene(this);
  setScene(scene);
  createGrid();

  connect(&animation_timer, &QTimer::timeout, this,
          &GridWidget::updateAnimation);
  animation_timer.setInterval(16); // ~60 FPS
}

void GridWidget::createGrid() {
  cells.resize(rows);
  for (int i = 0; i < rows; ++i) {
    cells[i].resize(cols);
    for (int j = 0; j < cols; ++j) {
      ColorCell *cell =
          new ColorCell(i, j, QColor(255, 255, 255)); // White background
      scene->addItem(cell);
      cells[i][j] = cell;
    }
  }
  resizeGrid();
}

void GridWidget::resizeGrid() {
  QRectF rect = viewport()->rect();

  qreal cell_width = rect.width() / cols;
  qreal cell_height = rect.height() / rows;

  qreal cell_size = qMin(cell_width, cell_height);

  qreal total_width = cols * cell_size;
  qreal total_height = rows * cell_size;

  qreal center_x = rect.width() / 2;
  qreal center_y = rect.height() / 2;

  qreal top_left_x = center_x - total_width / 2;
  qreal top_left_y = center_y - total_height / 2;

  setSceneRect(top_left_x, top_left_y, total_width, total_height);

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      cells[i][j]->setRect(0, 0, cell_size, cell_size);
      cells[i][j]->setPos(top_left_x + j * cell_size,
                          top_left_y + i * cell_size);
    }
  }
}

void GridWidget::setBackgroundImage(const QString &image_path) {
  this->image_path = "~/Desktop/Neo Se/6_14_2024_slice1_pic_cropped.jpg";
  QPixmap pixmap(image_path);

  qreal scale_x = sceneRect().width() / pixmap.width();
  qreal scale_y = sceneRect().height() / pixmap.height();

  QTransform transform;
  transform.scale(scale_x, scale_y);

  QBrush brush(pixmap);
  brush.setTransform(transform);

  setBackgroundBrush(brush);
}

void GridWidget::set_is_recording_video(bool value) {
  is_recording_video = value;
  for (auto &row : cells) {
    for (auto &cell : row) {
      cell->is_recording_video = value;
    }
  }
}

void GridWidget::hide_all_selected_tooltips() {
  for (auto &row : cells) {
    for (auto &cell : row) {
      cell->hideSelectedTooltip();
    }
  }
}

void GridWidget::cell_mouse_press_event(QGraphicsSceneMouseEvent *event) {
  if (!is_recording_video && event->button() == Qt::LeftButton) {
    QGraphicsItem *item = scene->itemAt(event->scenePos(), QTransform());
    if (ColorCell *cell = dynamic_cast<ColorCell *>(item)) {
      cell->clicked_state = !cell->clicked_state;
      emit cell_clicked(cell->row, cell->col);
      cell->update();
      if (cell->clicked_state) {
        hide_all_selected_tooltips();
        cell->showSelectedTooltip();
      } else {
        cell->hideSelectedTooltip();
        cell->hideHoverTooltip();
      }
    }
  }
}

void GridWidget::resizeEvent(QResizeEvent *event) {
  QGraphicsView::resizeEvent(event);
  resizeGrid();
  if (!image_path.isEmpty()) {
    setBackgroundImage(image_path);
  }
}

void GridWidget::contextMenuEvent(QContextMenuEvent *event) {
  QMenu context_menu(this);
  QAction *save_video_action = context_menu.addAction("Save as video");
  QAction *save_image_action = context_menu.addAction("Save as image");

  connect(save_video_action, &QAction::triggered, this,
          &GridWidget::save_as_video_requested);
  connect(save_image_action, &QAction::triggered, this,
          &GridWidget::save_as_image_requested);

  context_menu.exec(event->globalPos());
}

void GridWidget::leaveEvent(QEvent *event) {
  QGraphicsView::leaveEvent(event);
  for (auto &row : cells) {
    for (auto &cell : row) {
      cell->update();
    }
  }
}

void GridWidget::setCellOpacity(qreal opacity) {
  this->opacity = opacity;
}

void GridWidget::startAnimation() { animation_timer.start(); }

void GridWidget::stopAnimation() { animation_timer.stop(); }

void GridWidget::updateAnimation() {
  animation_phase += 0.1; // Adjust this value to change the animation speed
  if (animation_phase > 2 * M_PI) {
    animation_phase -= 2 * M_PI;
  }

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      qreal x = j / static_cast<qreal>(cols);
      qreal y = i / static_cast<qreal>(rows);

      // Create a radial wave pattern
      qreal distance = std::sqrt(std::pow(x - 0.5, 2) + std::pow(y - 0.5, 2));
      qreal wave = 0.5 * (std::sin(distance * 10 - animation_phase) + 1);

      // Map the wave value to a color (you can adjust this to create different
      // color patterns)
      int red = static_cast<int>(255 * wave);
      int green = static_cast<int>(255 * (1 - wave));
      int blue = static_cast<int>(128 + 127 * std::sin(wave * M_PI));

      cells[i][j]->setColor(QColor(red, green, blue), 1.0, opacity);
    }
  }

  scene->update();
}
