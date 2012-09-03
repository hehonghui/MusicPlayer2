#include <QtGui/QApplication>
#include <QTextCodec>
#include "mediaplayer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextCodec::setCodecForTr(QTextCodec::codecForLocale());
    MediaPlayer w;

    w.show();

    return a.exec();
}
