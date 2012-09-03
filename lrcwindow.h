#ifndef LRCWINDOW_H
#define LRCWINDOW_H

#include <QLabel>
#include <QTimer>

class lrcWindow : public QLabel
{
    Q_OBJECT
public:
    lrcWindow(QWidget *parent);
    QAction *exit;

    void setLrcWidth();

private:
    QPoint dragPosition;

    qreal length;
    qreal lrcWidth;

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void contextMenuEvent(QContextMenuEvent *ev);

    void paintEvent(QPaintEvent *);

};

#endif // LRCWINDOW_H
