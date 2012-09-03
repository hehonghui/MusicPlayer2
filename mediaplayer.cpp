#include "mediaplayer.h"
#include "ui_mediaplayer.h"

MediaPlayer::MediaPlayer(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    this->setWindowOpacity(1);                      // 设置淡入效果
    this->setWindowFlags(Qt::FramelessWindowHint);  // 总是显示在前 加Qt::WindowStaysOnTopHint
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground,true);
    this->setAcceptDrops(true);                           // 支持拖放

    codec = QTextCodec::codecForName("gb18030");    //  设置播放列表读取的文字编码
    bSize = true;
    bLrcShow = true;                                // 歌词默认不显示
    bSwitchDesLrc = true;                           // 桌面歌词显示的控制变量
    bDesLrc = true;
    bDoubleClicked = false;                         // 双击的时候显示歌词的控制

    currentRow = 0;
    DesLrcLabel = new lrcWindow(0);                 // 桌面歌词
    DesLrcLabel->show();                            // hide at first

    initWidget();                                   // 初始化 mediaObject 和 audioObject等
    createLayout();                                 // 布局
    createSystemTray();                             // 创建系统托盘图标啥的  在退出事件时才调用
    setButtonDisenable();                           // 设置按钮不可用
    readMusicList();                                // 从文件读播放列表
    doAnimation();                                  // 启动的动画

}

MediaPlayer::~MediaPlayer()
{
    delete DesLrcLabel;
}

void MediaPlayer::initWidget()
{

    QPalette p;
    p.setColor(QPalette::WindowText, QColor(46,46,46));
    showLabel->setPalette(p);
    showLabel->setFont(QFont(tr("Times New Roman"),9));

    // 要创建的audioOutput对象叫做音频接收槽直接与音频驱动器通信的层的组成部分并充当MediaObject的虚拟音频设备。
    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory,this); //
    mediaObject = new Phonon::MediaObject(this);        //
    metaInfomationResolver  = new Phonon::MediaObject(this);    // 指向当前音频文

    audioOutputPath = Phonon::createPath(mediaObject,audioOutput);    // 接收槽和媒体对象之间建立了连接
    effectDescriptions = Phonon::BackendCapabilities::availableAudioEffects();
    audioEffect = NULL;

    // 与自动下一曲的链接
    connect(mediaObject,SIGNAL(aboutToFinish()),this,SLOT(aboutToFinish()));
    // 与播放结束的链接  finished重载了   实现循环播放
    connect(mediaObject,SIGNAL(finished()),this,SLOT(finished()));
    // LCDNumber 每秒更新显示 和 获取当前歌曲总时间   类的私有槽函数
    connect(mediaObject,SIGNAL(tick(qint64)),this,SLOT(tickAndTotalTime(qint64)));
    connect(mediaObject,SIGNAL(tick(qint64)),this,SLOT(showDesktopLRC(qint64))); // 桌面歌词
    // 媒体元状态改变链接
    connect(metaInfomationResolver,SIGNAL(stateChanged(Phonon::State,Phonon::State)),
            this, SLOT(metaStateChanged(Phonon::State, Phonon::State)));
    // 媒体源改变链接  类的私有槽函数  实现资源的同步改变
    connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
             this, SLOT(sourceChanged(const Phonon::MediaSource &)));

    mediaObject->setTickInterval(1000);        // 设置1S钟更新一次时间  

    /*  ********************* 窗体控件  *************************** */
    // 进度条
    seekSlider = new Phonon::SeekSlider(this);    //  进度条
    seekSlider->setMediaObject(mediaObject);

     // 音量条
    volumeSlider = new Phonon::VolumeSlider(this);
    volumeSlider->setAudioOutput(audioOutput);   //  输出设备      音量条
    volumeSlider->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);  //  音量范围

    musicTimeLabel = new QLabel(tr("00:00/00:00"),this);
    musicTimeLabel->setFont(QFont(tr("Times New Roman"),16));
    musicTimeLabel->show();

    // lrc 歌词按钮
    lrcBtn = new QToolButton(this);
    lrcBtn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    lrcBtn->setAutoRaise(true);
    lrcBtn->setAutoRepeat(true);
    lrcBtn->setToolTip(tr("显示歌词"));
    lrcBtn->setIcon(QIcon(":/images/lrc.png"));
    lrcBtn->setEnabled(false);
    connect(lrcBtn,SIGNAL(clicked()), this, SLOT(showLRC()));

    // 歌词浏览框
    lrcTextBrowser = new QTextBrowser;           // 歌词显示框
    lrcTextBrowser->setWindowTitle(tr("LRC Window V1.0"));
    lrcTextBrowser->setAcceptDrops(true);
    lrcTextBrowser->setAcceptRichText(true);
    lrcTextBrowser->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    lrcTextBrowser->setFixedSize(260,320);
    lrcTextBrowser->setReadOnly(true);
    lrcTextBrowser->setFont(QFont(tr("Times New Roman"),11));
    lrcTextBrowser->setWindowFlags(Qt::WindowStaysOnTopHint);
    lrcTextBrowser->setStyleSheet("QTextBrowser { color: rgb(127, 0, 63);"
                                 "background-color: rgb(165,146,133);}");

    // 关于对话框
    aboutBtn = new QToolButton(this);
    aboutBtn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    aboutBtn->setAutoRaise(true);
    aboutBtn->setAutoRepeat(true);
    aboutBtn->setToolTip(tr("About The MusicPlayer"));
    aboutBtn->setIcon(QIcon(":/images/logo.png"));
    connect(aboutBtn,SIGNAL(clicked()), this, SLOT(about()));
    connect(aboutQtBtn, SIGNAL(clicked()),qApp,SLOT(aboutQt()));     // 关于Qt

}


// create layout and connection
void MediaPlayer::createLayout()
{

    /***********************   音乐列表    **************************/
   // QStringList headers;
    //headers << tr("Title");

    // 乐音列表 tableWidget
    musicTable = new QTableWidget(0,1);
    musicTable->setAcceptDrops(false);      // 不支持拖放
    musicTable->setStyleSheet("QTableWidget {border-image: url(:/images/tablebg.png);"
                              "background-color:rgba(200,255,255,255);"
                              "selection-background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                              "stop:0 #eccad7, stop: 0.5 #f7b6cf,"
                             "stop: 0.6 #f9a2c3, stop:1 #FF92BB);}");

    musicTable->verticalHeader()->setStyleSheet("QHeaderView::section {background: rgb(207,206,222);"
                                                "padding-left: 4px;border: 1px solid #6c6c6c;}");
    musicTable->horizontalHeader()->hide();             // 水平标题栏隐藏

    musicTable->horizontalHeader()->resizeSection(0,150);
    musicTable->setShowGrid(false);
    musicTable->setFont(QFont(tr("Times New Roman"),10));
    //musicTable->setHorizontalHeaderLabels(headers);     //  设置标题
    musicTable->setSelectionMode(QAbstractItemView::SingleSelection);   //  单选
    musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);    // 选择行
    // 建立连接  实现双击列表时播放音乐
    connect(musicTable,SIGNAL(cellDoubleClicked(int,int)),
            this,SLOT(tableWidget_cellDoubleClicked(int,int)));


    /*************************  布局管理    ******************************/
    QHBoxLayout *toolsBtnLayout = new QHBoxLayout;
    toolsBtnLayout->addStretch(20);
    toolsBtnLayout->addWidget(smallBtn);
    toolsBtnLayout->addWidget(expendBtn);
    toolsBtnLayout->addWidget(closeBtn);

    // 操作按钮布局
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(openBtn);
    btnLayout->addWidget(playBtn);
    btnLayout->addWidget(prevBtn);
    btnLayout->addWidget(nextBtn);

    //  音量、LCD时间、关于
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(volumeSlider);
    hlayout->addWidget(musicTimeLabel);
    hlayout->addWidget(lrcBtn);

    // 进度条
    QHBoxLayout *seekSliderLayout = new QHBoxLayout;
    seekSliderLayout->addWidget(seekSlider);
    seekSliderLayout->addWidget(aboutBtn);

    /***********************   核心布局    *****************************/
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(toolsBtnLayout);
  //  mainLayout->addLayout(playing);
    mainLayout->addLayout(btnLayout);
    mainLayout->addLayout(seekSliderLayout);
    mainLayout->addLayout(hlayout);
    mainLayout->addWidget(musicTable);

    setLayout(mainLayout);

}

// 绘制事件
void MediaPlayer::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    // 设置渲染  反锯齿 高质量反锯齿
    painter.setRenderHints(QPainter::Antialiasing|QPainter::HighQualityAntialiasing);
    painter.setBrush(QColor(212,212,212));
    painter.drawRoundedRect(rect(),10,10,Qt::AbsoluteSize);

}

// 拖事件
void MediaPlayer::dragEnterEvent(QDragEnterEvent *event)
 {

     setBackgroundRole(QPalette::Highlight);

     event->acceptProposedAction();
     emit changed(event->mimeData());
 }

 void MediaPlayer::dragMoveEvent(QDragMoveEvent *event)
 {
     event->acceptProposedAction();
 }

 // drop事件
 void MediaPlayer::dropEvent(QDropEvent *event)
 {

     QList<QUrl> urls = event->mimeData()->urls();
     if(urls.isEmpty())
     {
         return ;
     }
     QString fileName = urls.first().toLocalFile();
     if(fileName.isEmpty())
     {
         return ;
     }

     fileName.replace('/', '\\');
     QString musicFileName = fileName.right(fileName.length() - fileName.lastIndexOf('/') - 1);
     if(fileName.contains('\\'))
     {
        musicFileName = fileName.right(fileName.length() - fileName.lastIndexOf('\\') - 1);
     }

     // 像音乐列表中添加新项
     QTableWidgetItem *newItem = new QTableWidgetItem(musicFileName);
     int rowIndex = musicTable->rowCount();
     musicTable->insertRow(rowIndex);             // 新插入一行,再添加item
     musicTable->setItem(rowIndex, 0, newItem);     // 插入到列表中
     newItem->setFlags(newItem->flags() ^ Qt::ItemIsEditable);   //  可编辑的

     // 像音乐资源sources中添加新项
     Phonon::MediaSource musicItem(fileName);       // 创建一个新的音乐
     sources.append( musicItem );                   //  添加到播放列表

     addFileToList(fileName);                       // 写入文件

     setBackgroundRole(QPalette::NoRole);
     event->acceptProposedAction();
 }

 // 拖放操作,将歌曲添加到文件列表中
 void MediaPlayer::addFileToList(const QString &musicTitle)
 {
     QString AbsolutePath = QCoreApplication::applicationDirPath();
     QString fileName = "musicList.txt";
     QString pathName = AbsolutePath + "/" + fileName;
     qDebug() << pathName;
     QFile file( pathName );

     if(!file.open(QIODevice::WriteOnly | QIODevice::Append)){
         return ;
     }

     QTextStream ListStream(&file);
     ListStream.setAutoDetectUnicode(false);
     ListStream.setCodec(codec);
     ListStream << musicTitle<<"\n";     // 将文件名信息写入到文件

     file.close();
 }

 void MediaPlayer::dragLeaveEvent(QDragLeaveEvent *event)
 {
     event->accept();
 }



 //滚轮事件
void MediaPlayer::wheelEvent(QWheelEvent *wheelevent)
{
    if(wheelevent->delta() > 0 )    //上滚
    {
        qreal newVolume = audioOutput->volume() + (qreal)0.05;
         if(newVolume >= (qreal)1)
                newVolume = (qreal)1;
          audioOutput->setVolume(newVolume);
    }
    else    //下滚
    {
            qreal newVolume = audioOutput->volume() - (qreal)0.05;
            if(newVolume <= (qreal)0)
                newVolume = (qreal)0;
            audioOutput->setVolume(newVolume);
    }

}

// 创建系统托盘图片、菜单
void MediaPlayer::createSystemTray()
{

    showDesktopLRCAction = new QAction(QIcon(":/images/showlrc.png"),tr("隐藏桌面歌词"),this);
    connect(showDesktopLRCAction,SIGNAL(triggered()),this,SLOT(showDesLrc()));
    // play
    playAction = new QAction(QIcon(":/images/play.png"),tr("播放"),this);
    connect(playAction,SIGNAL(triggered()),this,SLOT(on_playBtn_clicked()));
    //  next track
    nextTrackAction = new QAction(QIcon(":/images/next.png"),tr("下一曲"),this);
    connect(nextTrackAction,SIGNAL(triggered()),this,SLOT(on_nextBtn_clicked()));
    // previous track
    prevTrackAction = new QAction(QIcon(":/images/prev.png"),tr("上一曲"),this);
    connect(prevTrackAction,SIGNAL(triggered()),this,SLOT(on_prevBtn_clicked()));

    removeAction = new QAction(QIcon(":/images/remove.png"),tr("移除本曲"),this);
   // removeAction->setEnabled(false);
    connect(removeAction,SIGNAL(triggered()),this,SLOT(removeCurrentMusic()));

    removeAllAction = new QAction(QIcon(":/images/clear.png"),tr("清空列表"),this);
    connect(removeAllAction,SIGNAL(triggered()),this,SLOT(removeMusicList()));

    // 显示
    ShowAction = new QAction(QIcon(":/images/logo.png"),tr("显示播放器"),this);
    connect(ShowAction,SIGNAL(triggered()),this,SLOT(showWidget()));
    // 系统托盘图标
    hideTrayIconAction = new QAction(QIcon(":/images/collapse.png"),tr("隐藏菜单"),this);
    connect(hideTrayIconAction,SIGNAL(triggered()),this,SLOT(hideTrayIconSlot()));

    // 退出
    exitAction = new QAction(QIcon(":/images/exitlogo.png"),tr("退出"),this);
    connect(exitAction,SIGNAL(triggered()),this,SLOT(close()));

    /** *系统托盘图标*  ***/
    trayicon = new QSystemTrayIcon(QIcon(":/images/logo.png"),this);
    // 建立连接  看文档的 activated(QSystemTrayIcon::ActivationReason) is a signal,
    // after doubleClicked is show the widget,但是signal和slot参数不一致，导致双击无法显示widget
    connect(trayicon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this,SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    trayiconMenu = new QMenu(this);
    trayiconMenu->addAction(playAction);

    trayiconMenu->addAction(nextTrackAction);
    trayiconMenu->addAction(prevTrackAction);
    trayiconMenu->addAction(ShowAction);
    trayiconMenu->addSeparator();
    trayiconMenu->addAction(hideTrayIconAction);
    trayiconMenu->addAction(exitAction);

    trayicon->setToolTip(tr("播放中..."));
    trayicon->setContextMenu(trayiconMenu);

    trayicon->show();
    trayicon->showMessage(tr("MusicPlayer"),tr("Mr.Simple's MusicPlayer"),QSystemTrayIcon::Information,10000);

}

// 右键菜单
void MediaPlayer::contextMenuEvent(QContextMenuEvent *e)
{
    // 显示比例菜单 使用动作组 QActionGroup
    QMenu *audioEffectMenu = new QMenu(tr("音效"),this);
    audioEffectMenu->setIcon(QIcon(":images/logo.png"));

    QActionGroup *audioEffectGroup = new QActionGroup(audioEffectMenu);   // 属于aspectMenu的动作组
    // 设置QAction组与槽函数的连接
    connect(audioEffectGroup, SIGNAL(triggered(QAction*)), this,
                                        SLOT(audioEffectChanged(QAction*)));
    audioEffectGroup->setExclusive(true);

    // 动作菜单
    Param_EQ = audioEffectMenu->addAction(tr("ParamEQ"));  // 添加动作到菜单
    Param_EQ->setCheckable(true);
    audioEffectGroup->addAction(Param_EQ);

    Normal = audioEffectMenu->addAction(tr("Normal"));
    Normal->setCheckable(true);
    Normal->setChecked(true);
    audioEffectGroup->addAction(Normal);

    AEC = audioEffectMenu->addAction(tr("AEC") );
    AEC->setCheckable(true);
    audioEffectGroup->addAction(AEC);

    wavesReverB = audioEffectMenu->addAction(tr("WavesReverb"));
    wavesReverB->setCheckable(true);
    audioEffectGroup->addAction(wavesReverB);

    Compressor = audioEffectMenu->addAction(tr("Compressor"));
    Compressor->setCheckable(true);
    audioEffectGroup->addAction(Compressor);

    xna = audioEffectMenu->addAction(tr("XnaVisualizerDmo"));
    xna->setCheckable(true);
    audioEffectGroup->addAction(xna);

    Echo = audioEffectMenu->addAction(tr("Echo"));
    xna->setCheckable(true);
    audioEffectGroup->addAction(Echo);

    Reverb = audioEffectMenu->addAction(tr("I3DL2Reverb"));
    Reverb->setCheckable(true);
    audioEffectGroup->addAction(Reverb);

    Flanger = audioEffectMenu->addAction(tr("Flanger"));
    Flanger->setCheckable(true);
    audioEffectGroup->addAction(Flanger);

    Chorus = audioEffectMenu->addAction(tr("Chorus"));
    Chorus->setCheckable(true);
    audioEffectGroup->addAction(Chorus);

    Resampler = audioEffectMenu->addAction(tr("Resampler DMO"));
    Resampler->setCheckable(true);
    audioEffectGroup->addAction(Resampler);

    // 右键菜单项
    QMenu *popMenu = new QMenu(musicTable);
    popMenu->addAction(showDesktopLRCAction);
    popMenu->addAction(playAction);
    popMenu->addAction(nextTrackAction);
    popMenu->addAction(prevTrackAction);
    popMenu->addMenu(audioEffectMenu);
    popMenu->addAction(removeAction);
    popMenu->addAction(removeAllAction);
    popMenu->addAction(exitAction);

    popMenu->exec(QCursor::pos());
}

// 设置按钮不可用
void MediaPlayer::setButtonDisenable()
{

    playBtn->setEnabled(false);
    nextBtn->setEnabled(false);
    prevBtn->setEnabled(false);
    playAction->setEnabled(false);
    nextTrackAction->setEnabled(false);
    prevTrackAction->setEnabled(false);
    showDesktopLRCAction->setEnabled(false);
}

// 设置按钮可用
void MediaPlayer::setButtonEnable()
{
    playBtn->setEnabled(true);         // 播放按钮等启用
    prevBtn->setEnabled(true);
    nextBtn->setEnabled(true);
    playAction->setEnabled(true);
    nextTrackAction->setEnabled(true);
    prevTrackAction->setEnabled(true);
    showDesktopLRCAction->setEnabled(true);
    lrcBtn->setEnabled(true);
}

// 读取播放列表
void MediaPlayer::readMusicList()
{

    QFile srcFile("musicList.txt");
    if(!srcFile.open(QIODevice::ReadOnly)){
        return ;
    }
    int index = sources.size();

    QTextStream in(&srcFile);
    in.setCodec(codec);     // 设置编码

    while(!in.atEnd()){     // 没有读完则继续读取   读行操作
        QString musicList = in.readLine();     // read all

        // 将读出的内容设置为媒体源
        Phonon::MediaSource newSource(musicList);
        sources.append(newSource);              // 追加到类成员变量sources里
        metaInfomationResolver->setCurrentSource(sources.at(index));
        mediaObject->setCurrentSource(sources.at(index));

        if(musicList.length() > 5){
            setButtonEnable();          // 按钮可用
        }

    }
    srcFile.close();
}

// 桌面歌词
void MediaPlayer::showDesLrc()
{
    if(bSwitchDesLrc){
         DesLrcLabel->hide();
         showDesktopLRCAction->setText(tr("隐藏桌面歌词"));
         bSwitchDesLrc = false;
    }
    else{
        DesLrcLabel->show();
        showDesktopLRCAction->setText(tr("显示桌面歌词"));
        bSwitchDesLrc = true;
    }

}

// 移除当前播放列表
void MediaPlayer::removeCurrentMusic()
{
    int key = QMessageBox::warning(this, tr("提示"),tr("确定移除当前歌曲吗?"),
                                   QMessageBox::Yes|QMessageBox::No);

    if(key == QMessageBox::Yes){                // 移除本曲

        QString title = musicTable->currentItem()->text();
        int currentRow = musicTable->currentRow();
        musicTable->removeRow(currentRow);
        sources.removeAt(currentRow);           // 移除完毕

        QList<QString> musicList;               // 存储歌曲列表
        readMusicListOnly(musicList, title);    // 读出歌曲到QList<QString>中
        writeMusicListOnly(musicList);          // 重写播放列表到文件中
    }
    else
    {
        return ;
    }

}

// 只读列表到一个QList<QString>中
void MediaPlayer::readMusicListOnly(QList<QString>& musicList, QString title)
{

    QFile file("musicList.txt");
    if(!file.open(QIODevice::ReadOnly)){
        return ;
    }
    QTextStream in(&file);
    in.setCodec(codec);                 // 设置编码

    while(!in.atEnd()){                 // 没有读完则继续读取   读行操作
        QString temp = in.readLine();
        if(temp.contains(title))        // 跳过选中的歌曲
        {
            continue;
        }
        musicList<<temp<<"\n";
    }
    file.close();
}

// 只将音乐列表写进文件中
void MediaPlayer::writeMusicListOnly(const QList<QString> musicList)

{
    QFile file("musicList.txt");
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        return ;
    }

    QTextStream ListStream(&file);
    foreach(QString musicTitle, musicList){

        /***************  保存播放列表  *****************/
        ListStream.setAutoDetectUnicode(false);
        ListStream.setCodec(codec);
        // 将列表输出到musicList.txt 保存  musicListString里只是保存了文件路径和文件名
        ListStream << musicTitle;     // 将文件名信息输出到文件
    }
    file.close();
}


// 清空播放列表
void MediaPlayer::removeMusicList()//清空列表
{
    mediaObject->pause();
    QList<Phonon::MediaSource> emptySource;
    sources = emptySource;   // 置空媒体资

    int j = musicTable->rowCount();
    for(int i= 0; i< j; i++)
    {
        musicTable->removeRow(0);
    }
    QFile file("musicList.txt");
    if(!file.open(QIODevice::WriteOnly)){
        QMessageBox::warning(this,tr("Error"),tr("Open Music List Error"),QMessageBox::Yes);
        return ;
    }

    QTextStream out(&file);
    out.setAutoDetectUnicode(false);
    out.setCodec(codec);
    out << "";
    file.close();

    playBtn->setIcon(QIcon(":/images/play.png"));
    playBtn->setIconSize(QSize(48,48));     // 设置ICON的大小
    playBtn->setToolTip(tr("Play"));
    playBtn->setEnabled(false);
    nextBtn->setEnabled(false);
    prevBtn->setEnabled(false);
    playAction->setEnabled(false);
    nextTrackAction->setEnabled(false);
    prevTrackAction->setEnabled(false);
    showDesktopLRCAction->setEnabled(false);
    DesLrcLabel->setText("Music...");
}

// 音效设置
void MediaPlayer::audioEffectChanged(QAction *act)
{
    mediaObject->pause();
    Normal->setChecked(false);
    if(act->text() == tr("ParamEq")){
       audioOutputPath.removeEffect(audioEffect);
       audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[0],this);
       audioOutputPath.insertEffect( audioEffect );
       Param_EQ->setChecked(true);

    }
    else if (act->text() == tr("Normal")){
        audioOutputPath.removeEffect( audioEffect );
        Normal->setChecked(true);
    }
    else if (act->text() == tr("AEC")){
        audioOutputPath.removeEffect(audioEffect);
        audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[1],this);
        audioOutputPath.insertEffect( audioEffect );
        AEC->setChecked(true);
    }
    else if (act->text() == tr("WavesReverb")){
         audioOutputPath.removeEffect(audioEffect);
         audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[2],this);
         audioOutputPath.insertEffect( audioEffect );
         wavesReverB->setChecked(true);

    }
    else if(act->text() == tr("XnaVisualizerDmo")){
        audioOutputPath.removeEffect(audioEffect);
        audioOutputPath.insertEffect(new
                                     Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[3],this));
        xna->setChecked(true);
    }
    else if(act->text() == tr("Compressor")){
        audioOutputPath.removeEffect(audioEffect);
        audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[5],this);
        audioOutputPath.insertEffect( audioEffect );
        Compressor->setChecked(true);
    }
    else if(act->text() == tr("Echo")){
        audioOutputPath.removeEffect(audioEffect);
        audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[7],this);
        audioOutputPath.insertEffect( audioEffect );
        Echo->setChecked(true);

    }
    else if(act->text() == tr("I3DL2Reverb")){
        audioOutputPath.removeEffect(audioEffect);
        audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[8],this);
        audioOutputPath.insertEffect( audioEffect );
        Reverb->setChecked(true);
    }
    else if(act->text() == tr("Flanger")){
        audioOutputPath.removeEffect(audioEffect);
        audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[9],this);
        audioOutputPath.insertEffect( audioEffect );
        Flanger->setChecked(true);
    }

    else if(act->text() == tr("Chorus")){
        audioOutputPath.removeEffect(audioEffect);
        audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[10],this);
        audioOutputPath.insertEffect( audioEffect );
        Chorus->setChecked(true);
    }
    else if(act->text() == tr("Resampler DMO")){
        audioOutputPath.removeEffect(audioEffect);
        audioEffect = new Phonon::Effect(Phonon::BackendCapabilities::availableAudioEffects()[11],this);
        audioOutputPath.insertEffect( audioEffect );
        Resampler->setChecked(true);
    }

    mediaObject->play();

}

// 列表中得资源改变
void MediaPlayer::sourceChanged(const Phonon::MediaSource &source)
{
    musicTable->selectRow(sources.indexOf(source));
    musicTimeLabel->setText(tr("00:00/00:00"));
}

// 多媒体状态改变 参考qt例子
void MediaPlayer::metaStateChanged(Phonon::State newState, Phonon::State oldState)
{
    if(newState == Phonon::ErrorState)
      {
          QMessageBox::warning(this,tr("Error open file"),
                               metaInfomationResolver->errorString());
          while(!sources.isEmpty() &&
              !(sources.takeLast() == metaInfomationResolver->currentSource()));
                    {} ;    //  数据不为空和不到最后数据时   循环   空语句执行
              return ;
      }

    //  暂停和停止状态则直接返回
    if(newState != Phonon::StoppedState && newState != Phonon::PausedState)
        return ;

    //  若是媒体是无效的  直接返回
    if(metaInfomationResolver->currentSource().type() == Phonon::MediaSource::Invalid)
        return ;

    //  获取数据源   Returns the strings associated with the given key
    QMap<QString,QString> metaData = metaInfomationResolver->metaData();

   // musicFileName = metaData.value("TITLE");   //  标题栏  一般mp3格式很少有  ape一般有

    QString fileName = metaInfomationResolver->currentSource().fileName();
        // 去掉路径 比如获取的文件名为 "E:/Musics/知足.mp3"  则执行后为 "知足.mp3"
    musicFileName = fileName.right(fileName.length() - fileName.lastIndexOf('/') - 1);
    musicFilePath = fileName.left(fileName.length() - musicFileName.length());

    //  音乐表
    QTableWidgetItem *titleItem = new QTableWidgetItem(musicFileName);
    titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);   //  可编辑的
    QTableWidgetItem *artistItem = new QTableWidgetItem(metaData.value("ARTIST"));
    artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);

    //  构建媒体列表  音乐表
    currentRow = musicTable->rowCount();
    musicTable->insertRow(currentRow);
    musicTable->setItem(currentRow, 0, titleItem);

    if (musicTable->selectedItems().isEmpty()) {
        musicTable->selectRow(0);    // 选择第一行的音乐源
        mediaObject->setCurrentSource(metaInfomationResolver->currentSource());
    }

    Phonon::MediaSource source = metaInfomationResolver->currentSource();
    int index = sources.indexOf(metaInfomationResolver->currentSource()) + 1; // 选择后索引+1 指向了下一曲

    if (sources.size() > index) {     //  文件源的大小过大   则设置媒体源的索引为index
        metaInfomationResolver->setCurrentSource(sources.at(index));
    }
    else {
        musicTable->resizeColumnsToContents();
        if (musicTable->columnWidth(0) > 400)
                musicTable->setColumnWidth(0, 400);   // 设置了列的宽度
    }

    lrcHash.clear();       // 桌面歌词的问题
    lrcHash.squeeze();
    bDesLrc = true;
}


// 关闭事件  最小化到系统托盘   最小化按钮的槽
void MediaPlayer::closeEvent(QCloseEvent *e)
{
     if(!this->isVisible()){
        qApp->quit();
        return ;
    }
    qApp->quit();
}

void MediaPlayer::changeEvent(QEvent *e)
{
         switch (e->type()) {
             case QEvent::WindowStateChange:
                     if(isModal() || !isMinimized())
                           this->setWindowState(Qt::WindowActive);  //恢复窗口显示
                           this->repaint();
                           e->ignore();
            default:
                   break;
        }
}

// 双击系统托盘显示播放窗口   过滤掉了 单击和中键点击
void MediaPlayer::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::Trigger://单击
    case QSystemTrayIcon::MiddleClick:
        break;
    case QSystemTrayIcon::DoubleClick://双击
        this->setWindowState(Qt::WindowActive);//恢复窗口显示
        this->show();
        if(!bLrcShow){
            lrcTextBrowser->show();  // 歌词窗口显示
        }
        break;
    default:
        break;
    }
}

// 时间显示槽函数 LCDNumber显示 和 歌曲总时间显示   LCDNumber每秒更新
void MediaPlayer::tickAndTotalTime(qint64 time)
{
    // 当前歌曲的总时间
    qint64 tempTime = mediaObject->totalTime();
    QTime totalTime(0,(tempTime/60000)%60,(tempTime/1000)%60,tempTime%1000);    // 总时间
    QTime curTime(0,(time / 60000) % 60,(time / 1000) % 60,time %1000);    //  当前时间 与tick(qint64) 时间同步
    musicTimeLabel->setText(tr("%1/%2").arg(curTime.toString("mm:ss")).arg(totalTime.toString("mm:ss")));
    musicTimeLabel->update();
}

/* ************ 在无标题栏情况下  鼠标按下和移动这两个函数实现窗口的移动 *********** */
void MediaPlayer::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->globalPos();       // 鼠标的原始位置
    newPos = event->globalPos() - event->pos();         // 当前鼠标位置

}

void MediaPlayer::mouseMoveEvent(QMouseEvent *event)
{
    QPoint newPos0 = event->globalPos();
    QPoint finalPos = newPos + newPos0 - lastPos;
    move(finalPos);     // 窗口移动函数
}
/*  ******************************* 窗口的移动  ******************************/


/*  ************************** Animation functions  动画 **********************/
// 启动动画
void MediaPlayer::doAnimation()
{
        QPropertyAnimation *amt = new QPropertyAnimation(this,"geometry",this);
        amt->setDuration(2500);
        QRect originalRect = this->geometry();      // 原始坐标大小

        amt->setStartValue(originalRect);
        amt->setEndValue(QRect(950,40,280,400));
        amt->setEasingCurve(QEasingCurve::OutExpo);
        amt->start();
}

// 窗口的缩小与扩展
void MediaPlayer::windowExpand()
{
     QPropertyAnimation *amt = new QPropertyAnimation(this,"geometry",this);
     amt->setDuration(1400);
     QRect originalRect = this->geometry();
     amt->setStartValue(originalRect);

     if(bSize){     // 缩小
            amt->setEndValue(QRect(950,40,280,120));
            expendBtn->setToolTip(tr("Expand"));
            expendBtn->setIcon(QIcon(":/images/collapse.png"));
            expendBtn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            expendBtn->setIconSize(QSize(18,18));
            bSize = false;
            amt->setEasingCurve(QEasingCurve::InOutBack);   // 设置特效
            amt->start();      // start the animation
            musicTable->hide();
     }
     else{      // 扩展
         amt->setEndValue(QRect(950,40,280,400));
         expendBtn->setToolTip(tr("Collapse"));
         expendBtn->setIcon(QIcon(":/images/expand.png"));
         expendBtn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
         expendBtn->setIconSize(QSize(20,20));
         bSize = true;
         amt->setEasingCurve(QEasingCurve::OutBounce);      // 设置特效
         amt->start();      // start the animation
         musicTable->show();
     }

}
/*  **************************** Animation functions  end **************************/


// 添加文件按钮
void MediaPlayer::addFiles()
{
   QStringList files = QFileDialog::getOpenFileNames(this,tr("Open music files"),
                                                     QDesktopServices::storageLocation(QDesktopServices::MusicLocation),
                                                     tr("Music (*.mp3 *.wma *.ape *.wav)"));
    playBtn->setChecked(false);

    if(files.isEmpty())     // 为空  则返回
        return ;

    // 音乐列表文件,用于保存播放列表用
    QFile fileList("musicList.txt");

    if(!fileList.open(QIODevice::WriteOnly | QIODevice::Append)){
        return ;
    }

    QTextStream ListStream(&fileList);

    int index = sources.size();     // 类的私有成员变量 sources 存储多媒体资源信息

    foreach(QString fileName,files){

        Phonon::MediaSource NewSources(fileName);  // 当前添加的资源
        sources.append(NewSources);             //  将NewSources的数据链接到sources后面
        fileName.append("\n");

        /***************  保存播放列表  *****************/
        ListStream.setAutoDetectUnicode(false);
        ListStream.setCodec(codec);
        // 将列表输出到musicList.txt 保存  musicListString里只是保存了文件路径和文件名
        ListStream << fileName;     // 将文件名信息输出到文件
    }

    if(!sources.isEmpty()){             //  资源不为空
        mediaObject->stop();
        metaInfomationResolver->setCurrentSource(sources.at(index));   // 指向当前资源的指针
        mediaObject->setCurrentSource(metaInfomationResolver->currentSource());
        playBtn->setEnabled(true);
        nextBtn->setEnabled(true);
        prevBtn->setEnabled(true);
        playAction->setEnabled(true);

        nextTrackAction->setEnabled(true);
        prevTrackAction->setEnabled(true);
        showDesktopLRCAction->setEnabled(true);

    }
    else
         QMessageBox::warning(this,tr("Error"),tr("empty"),QMessageBox::Yes);
    getMusicPathAndName();
    fileList.close();       // 关闭文件
}

// 获取歌曲文件名和路径  以便查找歌词文件
void MediaPlayer::getMusicPathAndName()
{
    /* ***************** 重新获取歌曲名称和路径  以便显示歌词 ******************/
    QString fileName = mediaObject->currentSource().fileName();
    musicFileName = "";
    musicFilePath = "";
    musicFileName = fileName.right(fileName.length() - fileName.lastIndexOf('/') - 1);  // 音乐文件名
    musicFilePath = fileName.left(fileName.length() - musicFileName.length());      // 路径名
    QString showName = musicFileName.left(musicFileName.length() - 4);  // 获得歌曲实名
    showLabel->setText("Playing: " + showName);
}

// 双击音乐列表播放
void MediaPlayer::tableWidget_cellDoubleClicked(int row, int column)//双击选歌
{
    playBtn->setIcon(QIcon(":/images/push.png"));   // 暂停
    mediaObject->stop();
    mediaObject->clearQueue();      // 清空队列

    if (row >= sources.size())
        return;

    mediaObject->setCurrentSource(sources[row]);   // QStringList重载了[]符  选择当前曲目
    mediaObject->play();    // 播放

    getMusicPathAndName();    // 获取歌曲文件名和路径  以便查找歌词文件

    // 双击的时候 清空歌词
    lrcTextBrowser->clear();
    if(bDoubleClicked){       // 当歌词已经显示的时候再双击  则调用歌词显示
        bLrcShow = true;   // 显示以后设置  bLrcShow为true  再按一下歌词按钮 则为隐藏
        showLRC(); 
    }
    lrcHash.clear();
    lrcHash.squeeze();
     bDesLrc = true;

}

//  播放、暂停函数
void MediaPlayer::playOrPause()
{
    playAction->setIcon(QIcon(":/images/push.png"));
    playAction->setText(tr("暂停"));

    switch(mediaObject->state()){

    case Phonon::PlayingState:
        playBtn->setIcon(QIcon(":/images/play.png"));
        playBtn->setIconSize(QSize(48,48));     // 设置ICON的大小
        playBtn->setToolTip(tr("Play"));
        playAction->setIcon(QIcon(":/images/play.png"));
        playAction->setText(tr("播放"));
        mediaObject->pause();
        playBtn->setChecked(false);
        break;
    case Phonon::PausedState:
        playBtn->setIcon(QIcon(":/images/push.png"));   // 暂停
        playBtn->setToolTip(tr("Pause"));
        playAction->setIcon(QIcon(":/images/push.png"));
        playAction->setText(tr("暂停"));
        mediaObject->play();
        break;
    case Phonon::StoppedState:
        mediaObject->play();
        break;
    case Phonon::LoadingState:
        playBtn->setChecked(false);
        break;
    case Phonon::BufferingState:
        break;
    case Phonon::ErrorState:
            if(mediaObject->errorType()== Phonon::FatalError)
                QMessageBox::warning(this,tr("Fatal Error"),mediaObject->errorString());
            else
                QMessageBox::warning(this,tr("Error"),mediaObject->errorString());
            break;
    }

   getMusicPathAndName();   // 获取歌曲文件名和路径  以便查找歌词文件
}

// next  track
void MediaPlayer::nextMusic()
{
    // 当前资源索引加1 即下一曲索引位置
    int index = sources.indexOf(mediaObject->currentSource()) + 1;

    if(sources.size() > index){
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(index));        // 设置当前资源文件索引
        mediaObject->play();                                 // 播放
        musicTable->selectRow(index);
    }
    else{       // 此时为最后一曲  跳转到第一首
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(0));        // 设置当前资源文件索引
        mediaObject->play();
    }

    getMusicPathAndName();   // 获取歌曲文件名和路径  以便查找歌词文件
    if(!bLrcShow){        // 已经显示歌词窗口的情况下
         bLrcShow = true;
         showLRC();
    }


}

// previous track
void MediaPlayer::preMusic()
{
    int index = sources.indexOf(mediaObject->currentSource()) - 1;    // 获取上一曲索引位置
    if(index < 0){      // 此时指向了第一首  再点上一曲则跳到最后一曲
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(currentRow));    // 设置为最后一曲
        mediaObject->play();
        musicTable->selectRow(index);
    }
    else{
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(index));        // 设置当前资源文件索引
        mediaObject->play();            // 播放
    }

  getMusicPathAndName();   // 获取歌曲文件名和路径  以便查找歌词文件
  if(!bLrcShow){        // 已经显示歌词窗口的情况下
       bLrcShow = true;
       showLRC();
  }
}

// about to finish current track,自动进行下一曲
void MediaPlayer::aboutToFinish()
{
    int index = sources.indexOf(mediaObject->currentSource()) + 1;       // 获取下一曲索引位置
    if(sources.size() > index){
        mediaObject->enqueue(sources.at(index));    // 入队操作  当有曲目时进行下一曲 否则停止
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(index));
        mediaObject->play();     // 播放
    }
    else
        playBtn->setChecked(false);

    getMusicPathAndName();   // 获取歌曲文件名和路径  以便查找歌词文件
    if(!bLrcShow){        // 已经显示歌词窗口的情况下
         bLrcShow = true;
         showLRC();
    }

    lrcHash.clear();       // 桌面歌词的问题
    bDesLrc = true;
    showDesktopLRC(0);

}

// finished  整个文件源的结尾
void MediaPlayer::finished()        //  播放完毕后 从头开始循环播放
{
    this->show();
    this->setWindowState(Qt::WindowActive);//恢复窗口显示

    mediaObject->stop();
    mediaObject->setCurrentSource(sources.at(0));        // 设置当前资源文件索引
    mediaObject->play();

    getMusicPathAndName();   // 获取歌曲文件名和路径  以便查找歌词文件
    if(!bLrcShow){        // 已经显示歌词窗口的情况下
         bLrcShow = true;
         showLRC();
    }

}


/*   *********************   signals and slots   ******************************  */

void MediaPlayer::showLRC()
{
    QRect rect = this->geometry();
    lrcTextBrowser->setGeometry(rect.x()-300,rect.y()+100,rect.width(),rect.height());

    QFile file;     // 文件
    QDir::setCurrent(musicFilePath);        // 设置音乐文件路径为当前路径
    QString lrcFileName(musicFileName);     // 设置文件名为正在播放的文件名

    /*  ***********************************************************************
        将 .mp3文件名改成 .lrc  但是用QRegExp有局限性 一次只能去除一种格式的
        lrcFileName = lrcFileName.replace(QRegExp(".mp3"),".lrc");
        而 lrcFileName = lrcFileName.remove(musicFileName.right(3)) + "lrc";  全部格式都适用
    *****************************************************************************/

     lrcFileName = lrcFileName.remove(musicFileName.right(3)) + "lrc";
     file.setFileName(lrcFileName);

    QString lrcCache;        // 从文件中读进来的内容存进里面

    if(bLrcShow){      // 按钮实现显示歌词和隐藏功能

        QString lrc;
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)) // 叛断歌词文件是否打开
        {
           QTextStream lrcStream(&file);
           QTextCodec *codec = QTextCodec::codecForName("gb18030"); //解决乱码问题
           lrcStream.setCodec(codec);

           while(!(lrcStream.atEnd())){         // 没有读完的时候
              lrcCache = lrcStream.readLine();   // 读取每一行歌词文件的时间 和 歌词部分
              int numTimes = lrcCache.count(']');  // 获取该行歌词中有几个  ]
               // 截取时间  用 QMap组合起来 [00.16.23] 则截取到  00.16.23
              // QString strTime = lrcCache.mid(1,8);
              //   QString strTime2,strTime3;
              if(numTimes > 1){
                  if(lrcCache.endsWith("]"))
                      continue ;
                   else{
                      //strTime2 = lrcCache.mid(11,8);
                    //  lrcCache.replace(0,20,"");

                  }

              }
               lrcCache.replace(QRegExp("\\[\\d{2}:\\d{2}\\.\\d{2}\\]"),"");

               lrc.append(" " + lrcCache);
               lrc.append("\n\n");

           }
           lrcTextBrowser->setText(lrc);
           lrcTextBrowser->show();           // 显示歌词
           bLrcShow = false;        // 已经显示后设置为false
           bDoubleClicked = true;
           lrcBtn->setToolTip(tr("隐藏歌词"));
        }
        else{
             QSound::play("warning.wav");
             lrcTextBrowser->hide();
             lrcTextBrowser->clear();
             bLrcShow = true;
             bDoubleClicked = false;        // 双击是否显示的问题
             lrcBtn->setToolTip(tr("显示歌词"));
             QMessageBox::warning(this,tr("Warning"),
                                  tr("%1 没有歌词文件,请手动下载!").arg(musicFileName),
                                  QMessageBox::Yes);
            }
    }
    else{
        lrcTextBrowser->clear();
        lrcTextBrowser->hide();
        bLrcShow = true;
        bDoubleClicked = false;
        lrcBtn->setToolTip(tr("显示歌词"));
    }

    file.close();       // close the lrc file
}


// 桌面歌词
void MediaPlayer::showDesktopLRC(qint64 time)
{
    getMusicPathAndName();
 /*   bool bTwo = false;
    bool bThr = false;
    */

    if(lrcHash.isEmpty()){   // 换了歌曲的话　要重读文件
        DesLrcLabel->setText(tr("Music..."));
        DesLrcLabel->update();
        bDesLrc = true;
        lrcHash.squeeze();      // resize the hash<>
    }

    long timeFlag = (long)time;
    QString t = QString::number(timeFlag);
    if(timeFlag < 9000)
        t = t.mid(0,1);
    if(timeFlag > 9000 && timeFlag < 100000)
        t = t.mid(0,2);
    else
        t = t.mid(0,3);
    int realTime = t.toInt();

    QFile file;  // 歌词文件

    if(bDesLrc){   // 需要重读文件 bDesLrc是控制变量
        QDir::setCurrent(musicFilePath);        // 设置音乐文件路径为当前路径
        QString lrcFileName(musicFileName);     // 设置文件名为正在播放的文件名
        lrcFileName = lrcFileName.remove(musicFileName.right(3)) + "lrc";
        file.setFileName(lrcFileName);
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)) // 叛断歌词文件是否打开
            bDesLrc = false;

        }

        QString lrcCache = "";        // 从文件中读进来的内容存进里面   缓存

        QTextStream lrcStream(&file);
        QTextCodec *codec = QTextCodec::codecForName("gb18030"); //解决乱码问题
        lrcStream.setCodec(codec);
        // read the lrc stream
        while(!(lrcStream.atEnd())){       // 没有读完的时候

            int timeTwo = 0,timeThr = 0;
            bool bTwo = false;
            bool bThr = false;

            lrcCache = lrcStream.readLine();    // read line          
            if(lrcCache == "")  // the line is empty,skip
                  continue ;
            if(lrcCache.endsWith("]"))  // the line is end with "]" ,skip
                    continue ;

            int temp = lrcCache.mid(2,1).toInt();   // [00:45.34] 则分钟被转换为int 也是 0
            int iOne = lrcCache.mid(4,2).toInt();
            int itime = temp*60 + iOne;

            int numTimes = lrcCache.count(']');  // 获取该行歌词中有几个']'
            if(numTimes == 2){    // [02:48.01][01:10.00]没想到失去的勇气我还留着
                      bTwo = true;
                      int itwo = lrcCache.mid(12,1).toInt();
                      timeTwo = lrcCache.mid(14,2).toInt() + itwo*60;
                  }
            if(numTimes == 3){    // [03:40.00][03:02.00][01:30.00]风会带走一切短暂的轻松
                      bTwo = true;
                      bThr = true;
                      int itwo = lrcCache.mid(12,1).toInt();
                      timeTwo = lrcCache.mid(14,2).toInt() + itwo*60;
                      int ithr = lrcCache.mid(22,1).toInt();
                      timeThr = lrcCache.mid(24,2).toInt() + ithr*60;
                  }

            if(bThr)    // 含有三个时间 去掉前面三十个字符
               lrcCache.replace(0,30,"");
            else if(bTwo)   // 含有两个时间轴
                     lrcCache.replace(0,20,"");  // 替换法  replace

            // 去除歌词前面的时间 比如 [00:23.11]hello 变为 hello
            lrcCache.replace(QRegExp("\\[\\d{2}:\\d{2}\\.\\d{2}\\]"),""); // [03:40.00] 正则式去除法
            lrcHash.insert(itime, lrcCache);    // 插入QHash表
            if(bTwo)    // 有两个时间
                lrcHash.insert(timeTwo, lrcCache);
            if(bThr)    // 三个时间
                lrcHash.insert(timeThr, lrcCache);

        }  // 读取歌词结束

        file.close();    // close the file

        // 遍历哈希表  找出与时间线对应的歌词
        QHash<int,QString>::const_iterator iterator = lrcHash.constBegin();
        while (iterator != lrcHash.constEnd()) {
             if((iterator.key()) == realTime){  // 当前时间与哈希表钟的时间对已  相等则输出歌词
                  DesLrcLabel->setText(tr("%1").arg(iterator.value()));
                  break;
             }
             ++iterator;
         }
 }

void MediaPlayer::on_openBtn_clicked()
{
    addFiles();
}

void MediaPlayer::on_playBtn_clicked()
{
    playBtn->setIcon(QIcon(":/images/push.png"));
    playBtn->setToolTip(tr("Pause"));
    playOrPause();
}

void MediaPlayer::on_prevBtn_clicked()
{
    preMusic();
    getMusicPathAndName();   // 获取歌曲文件名和路径  以便查找歌词文件
    lrcHash.clear();
    lrcHash.squeeze();
    bDesLrc = true;

}

void MediaPlayer::on_nextBtn_clicked()
{
    nextMusic();
    getMusicPathAndName();   // 获取歌曲文件名和路径  以便查找歌词文件
    lrcHash.clear();       // 桌面歌词的问题
    lrcHash.squeeze();
    bDesLrc = true;
}

void MediaPlayer::on_closeBtn_clicked()
{
    close();        // 重载了  closeEvent
}

void MediaPlayer::on_expendBtn_clicked()
{
   windowExpand();
}

void MediaPlayer::on_smallBtn_clicked()
{

    this->setWindowState(Qt::WindowMinimized);
    if(!bLrcShow){
        lrcTextBrowser->hide();        // 隐藏歌词
    }

}

void MediaPlayer::hideTrayIconSlot()
{
    trayiconMenu->hide();
}

void MediaPlayer::showWidget()
{
    this->setWindowState(Qt::WindowActive);//恢复窗口显示
    this->show();
    if(!bLrcShow){        // 已经显示歌词窗口的情况下
         lrcTextBrowser->show();
    }
}

void MediaPlayer::about()
{
        QMessageBox::about(this, tr("Author's Mail: bboyfeiyu@gmail.com"),
                   tr("<h2>Designed By Mr.Simple UIT-211-WORKROOM</h2>"
                      "<p>Copyright &copy; 2011 Software Inc."
                      "<p>  Simple MusicPlayer is a small application that "
                      "can play musics,such as mp3,ape,wma and so on,it's "
                      "base on Phonon library  which is a cross-platform "
                      "Qt classe."));
}
