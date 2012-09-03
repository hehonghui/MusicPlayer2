#include "lrcwindow.h"
#include <QMouseEvent>
#include <QMenu>
#include <QTimer>

#include <QDebug>
#include <QPainter>
lrcWindow::lrcWindow(QWidget *parent):
      QLabel(parent)
{
    this->setWindowFlags(Qt::SubWindow |Qt::FramelessWindowHint| Qt::WindowStaysOnTopHint);
    this->resize(1024,80);
    this->setText(tr("Music ..."));
    this->setAttribute(Qt::WA_TranslucentBackground);//背景透明

    this->setCursor(Qt::OpenHandCursor);

    exit = new QAction(tr("隐藏(&D)"),this);
    connect(exit,SIGNAL(triggered()),this,SLOT(close()));

    this->move(580,620);

    lrcWidth = 0;

}

void lrcWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    else if(event->button() == Qt::MidButton)//点击鼠标滚轮
        close();
    QLabel::mousePressEvent(event);
}

void lrcWindow::mouseMoveEvent(QMouseEvent *e)
{
    if(e->buttons() & Qt::LeftButton)
    {
        move(e->globalPos() - dragPosition);
        e->accept();
    }
    QLabel::mouseMoveEvent(e);
}

void lrcWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    menu.addAction(exit);
    menu.exec(ev->globalPos());
    QLabel::contextMenuEvent(ev);
}

void lrcWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    //使用该行代码可以实现反锯齿，使歌词显示更漂亮，但是会明显增加CPU占用率
    QFont font(tr("Times New Roman"),20,QFont::Bold);
    painter.setFont(font);
    QLinearGradient lg(0,20,0,50);
    lg.setColorAt(0,QColor(0,170,255,255));
    lg.setColorAt(0.2,QColor(61,214,191,250));
    lg.setColorAt(0.5,QColor(85,255,255,255));
    lg.setColorAt(0.8,QColor(61,214,191,250));
    lg.setColorAt(1,QColor(0,170,255,255));
    painter.setBrush(lg);
    painter.setPen(Qt::NoPen);
    QPainterPath textPath;
    textPath.addText(0,50,font,text());
    painter.drawPath(textPath);

    length = textPath.currentPosition().x();

    painter.setPen(Qt::yellow);
    painter.drawText(0,14,lrcWidth,50,Qt::AlignLeft,text());

}

void lrcWindow::setLrcWidth()
{
    lrcWidth = 0;
}

