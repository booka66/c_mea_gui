#ifndef COLORCELL_H
#define COLORCELL_H

#include <QGraphicsRectItem>
#include <QObject>
#include <QColor>
#include <QLabel>
#include <QTimer>

class ColorCell : public QObject, public QGraphicsRectItem {
    Q_OBJECT

public:
    ColorCell(int row, int col, const QColor &color, QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void showHoverTooltip();
    void showSelectedTooltip();
    void hideSelectedTooltip();
    void hideHoverTooltip();
    void setColor(const QColor &color, qreal strength = 1.0, qreal opacity = 1.0);
    void setText(const QString &text);

    int row, col;
    bool clicked_state;
    bool plotted_state;
    QString plotted_shape;
    bool is_recording_video;

private:
    QColor hover_color;
    QColor selected_color;
    QColor plotted_color;
    int selected_width;
    int plotted_width;
    QLabel *selected_tooltip;
    QLabel *hover_tooltip;
    QTimer tooltip_timer;
    QString text;
};

#endif // COLORCELL_H
