#ifndef GRIDWIDGET_H
#define GRIDWIDGET_H

#include <QGraphicsView>
#include <QVector>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include "colorcell.h"

class GridWidget : public QGraphicsView {
    Q_OBJECT

public:
    GridWidget(int rows, int cols, QWidget *parent = nullptr);

    void setBackgroundImage(const QString &image_path);
    void set_is_recording_video(bool value);
    void hide_all_selected_tooltips();
    void startAnimation();
    void stopAnimation();
    void setCellOpacity(qreal opacity);

signals:
    void cell_clicked(int row, int col);
    void save_as_video_requested();
    void save_as_image_requested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void updateAnimation();

private:
    void createGrid();
    void resizeGrid();
    void cell_mouse_press_event(QGraphicsSceneMouseEvent *event);

    QGraphicsScene *scene;
    int rows, cols;
    QVector<QVector<ColorCell*>> cells;
    bool is_recording_video;
    ColorCell *selected_channel;
    QString image_path;
    QTimer animation_timer;
    qreal animation_phase;
    QGraphicsPixmapItem *background_image;
    qreal opacity;
};

#endif // GRIDWIDGET_H
