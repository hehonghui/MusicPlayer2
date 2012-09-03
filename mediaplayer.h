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
    void mousePressEvent(QMouseEvent *event);   // 鼠标单击事件
    void mouseMoveEvent(QMouseEvent *);   // 鼠标移动事件
    void paintEvent(QPaintEvent *);     // 绘制事件
    void closeEvent(QCloseEvent *);     // 关闭事件
    void wheelEvent(QWheelEvent *);     // 滚轮事件
    void contextMenuEvent(QContextMenuEvent *);     // 右键菜单
    void changeEvent(QEvent *);

    void dropEvent(QDropEvent *);
signals:
    void changed(const QMimeData *mimeData = 0);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);


    void createLayout();        // 创建布局
    void createSystemTray();            // 最小化到系统托盘
    void readMusicList();       // 读取播放列表

    void windowExpand();
    void doAnimation();
    void getMusicPathAndName();

private:
    void setButtonDisenable();  // 设置按钮不可用
    void setButtonEnable();     // 设置按钮可用
    void readMusicListOnly(QList<QString>& musicList,
                           QString title);                      // 读音乐列表
    void writeMusicListOnly(const QList<QString> musicList);    // 写音乐列表
    void addFileToList(const QString& title);                   // 将歌曲写到文件列表中

private slots:

    void addFiles();
    void playOrPause();
    void nextMusic();
    void preMusic();        // 上一曲
    void aboutToFinish();   // 每曲即将结束时 自动下一曲功能
    void finished();        // 播放完一曲
    void hideTrayIconSlot();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);   // 显示主程序
    void tableWidget_cellDoubleClicked(int row, int column);
    void metaStateChanged(Phonon::State newState, Phonon::State oldState);  // 状态改变
    void sourceChanged(const Phonon::MediaSource &source);      // 资源文件改变
    void showWidget();      // 显示窗口
    void about();       // about the music player
    void showLRC();     // textBrowser显示歌词
    void showDesktopLRC(qint64);            // 显示桌面歌词
    void tickAndTotalTime(qint64 time);    // 时间显示槽函数 LCDNumber显示 和 歌曲总时间显示
    void audioEffectChanged(QAction*);      // 设置音效
    void removeMusicList();      // 清除播放列表
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

    Phonon::SeekSlider *seekSlider;      //  时间轴  滑动条 即媒体的播放位置
    Phonon::VolumeSlider *volumeSlider;    //  音量控制条

    QList<Phonon::MediaSource> sources;
    Phonon::MediaObject *mediaObject;       // 多媒体对象
    Phonon::AudioOutput *audioOutput;       // 音频输出设备
    Phonon::MediaObject *metaInfomationResolver;    // 指向当前音频文件
    Phonon::Effect *audioEffect;                    // 音效
    Phonon::Path audioOutputPath;                   // 路径
    QList<Phonon::EffectDescription> effectDescriptions;    // 音效列表
    Phonon::EffectDescription effectDescription;            // 音效描述

    QPoint lastPos;     // 用于移动窗口 上一个的鼠标位置
    QPoint newPos;       // 用于移动窗口 当前的鼠标位置

    QTableWidget *musicTable;   // 音乐列表
    //  系统托盘图标
    QSystemTrayIcon *trayicon;
    QMenu *trayiconMenu;

    //  系统托盘的动作菜单
    QAction *showDesktopLRCAction;      // 显示桌面歌词
    QAction *playAction;
    QAction *nextTrackAction;
    QAction *prevTrackAction;
    QAction *removeAllAction;
    QAction *removeAction;
    QAction *ShowAction;
    QAction *hideTrayIconAction;
    QAction *exitAction;

    QAction *Param_EQ;                  // 所有音效的动作
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

    QLabel *musicTimeLabel;      // 显示总时间
    QTextBrowser *lrcTextBrowser;

    //******************************************

    QString musicFileName;          // 音乐文件名   这两个用于打开歌词的
    QString musicFilePath;      // 路径名
    QTextCodec *codec;
    lrcWindow *DesLrcLabel;
    QHash<int , QString> lrcHash;
    LRC lrcStruct[2000];

    bool bDesLrc;
    bool bSwitchDesLrc;
    bool bSize;         //  控制窗口大小
    bool bLrcShow;
    int currentRow;         // 列表的行数
    bool bDoubleClicked;
};

#endif // MEDIAPLAYER_H
