#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QWidget>
#include <QStringList>
#include <QList>
#include <QFileDialog>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLCDNumber>
#include <QTime>
#include <QIcon>
#include <QSound>
#include <QTextCodec>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QSize>
#include <QSizePolicy>
#include <QPainter>
#include <QTextBrowser>
#include <QPoint>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QRect>
#include <QPalette>
#include <QCloseEvent>
#include <QMessageBox>
#include <QAction>
#include <QActionGroup>
#include <phonon>
#include <QTableWidgetItem>
#include <QMap>
#include <QTableWidgetItem>
#include <lrcwindow.h>
#include "ui_mediaplayer.h"

typedef struct lrcStruct{
    QString time;
    QString works;
}LRC;

class MediaPlayer : public QWidget,public Ui::MediaPlayer
{
    Q_OBJECT

public:
    explicit MediaPlayer(QWidget *parent = 0);
    ~MediaPlayer();

private:
    void initWidget();
    void mousePressEvent(QMouseEvent *event);   // ��굥���¼�
    void mouseMoveEvent(QMouseEvent *);   // ����ƶ��¼�
    void paintEvent(QPaintEvent *);     // �����¼�
    void closeEvent(QCloseEvent *);     // �ر��¼�
    void wheelEvent(QWheelEvent *);     // �����¼�
    void contextMenuEvent(QContextMenuEvent *);     // �Ҽ��˵�
    void changeEvent(QEvent *);

    void dropEvent(QDropEvent *);
signals:
    void changed(const QMimeData *mimeData = 0);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);


    void createLayout();        // ��������
    void createSystemTray();            // ��С����ϵͳ����
    void readMusicList();       // ��ȡ�����б�

    void windowExpand();
    void doAnimation();
    void getMusicPathAndName();

private:
    void setButtonDisenable();  // ���ð�ť������
    void setButtonEnable();     // ���ð�ť����
    void readMusicListOnly(QList<QString>& musicList,
                           QString title);                      // �������б�
    void writeMusicListOnly(const QList<QString> musicList);    // д�����б�
    void addFileToList(const QString& title);                   // ������д���ļ��б���

private slots:

    void addFiles();
    void playOrPause();
    void nextMusic();
    void preMusic();        // ��һ��
    void aboutToFinish();   // ÿ����������ʱ �Զ���һ������
    void finished();        // ������һ��
    void hideTrayIconSlot();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);   // ��ʾ������
    void tableWidget_cellDoubleClicked(int row, int column);
    void metaStateChanged(Phonon::State newState, Phonon::State oldState);  // ״̬�ı�
    void sourceChanged(const Phonon::MediaSource &source);      // ��Դ�ļ��ı�
    void showWidget();      // ��ʾ����
    void about();       // about the music player
    void showLRC();     // textBrowser��ʾ���
    void showDesktopLRC(qint64);            // ��ʾ������
    void tickAndTotalTime(qint64 time);    // ʱ����ʾ�ۺ��� LCDNumber��ʾ �� ������ʱ����ʾ
    void audioEffectChanged(QAction*);      // ������Ч
    void removeMusicList();      // ��������б�
    void removeCurrentMusic();
    void showDesLrc();

    void on_openBtn_clicked();
    void on_playBtn_clicked();
    void on_prevBtn_clicked();
    void on_nextBtn_clicked();
    void on_closeBtn_clicked();
    void on_expendBtn_clicked();
    void on_smallBtn_clicked();

private:

    Phonon::SeekSlider *seekSlider;      //  ʱ����  ������ ��ý��Ĳ���λ��
    Phonon::VolumeSlider *volumeSlider;    //  ����������

    QList<Phonon::MediaSource> sources;
    Phonon::MediaObject *mediaObject;       // ��ý�����
    Phonon::AudioOutput *audioOutput;       // ��Ƶ����豸
    Phonon::MediaObject *metaInfomationResolver;    // ָ��ǰ��Ƶ�ļ�
    Phonon::Effect *audioEffect;                    // ��Ч
    Phonon::Path audioOutputPath;                   // ·��
    QList<Phonon::EffectDescription> effectDescriptions;    // ��Ч�б�
    Phonon::EffectDescription effectDescription;            // ��Ч����

    QPoint lastPos;     // �����ƶ����� ��һ�������λ��
    QPoint newPos;       // �����ƶ����� ��ǰ�����λ��

    QTableWidget *musicTable;   // �����б�
    //  ϵͳ����ͼ��
    QSystemTrayIcon *trayicon;
    QMenu *trayiconMenu;

    //  ϵͳ���̵Ķ����˵�
    QAction *showDesktopLRCAction;      // ��ʾ������
    QAction *playAction;
    QAction *nextTrackAction;
    QAction *prevTrackAction;
    QAction *removeAllAction;
    QAction *removeAction;
    QAction *ShowAction;
    QAction *hideTrayIconAction;
    QAction *exitAction;

    QAction *Param_EQ;                  // ������Ч�Ķ���
    QAction *Normal;
    QAction *AEC;
    QAction *wavesReverB;
    QAction *Compressor;
    QAction *xna;
    QAction *Echo;
    QAction *Reverb;
    QAction *Flanger;
    QAction *Chorus;
    QAction *Resampler;

    QToolButton *lrcBtn;
    QToolButton *aboutBtn;

    QLabel *musicTimeLabel;      // ��ʾ��ʱ��
    QTextBrowser *lrcTextBrowser;

    //******************************************

    QString musicFileName;          // �����ļ���   ���������ڴ򿪸�ʵ�
    QString musicFilePath;      // ·����
    QTextCodec *codec;
    lrcWindow *DesLrcLabel;
    QHash<int , QString> lrcHash;
    LRC lrcStruct[2000];

    bool bDesLrc;
    bool bSwitchDesLrc;
    bool bSize;         //  ���ƴ��ڴ�С
    bool bLrcShow;
    int currentRow;         // �б������
    bool bDoubleClicked;
};

#endif // MEDIAPLAYER_H
