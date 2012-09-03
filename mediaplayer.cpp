#include "mediaplayer.h"
#include "ui_mediaplayer.h"

MediaPlayer::MediaPlayer(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    this->setWindowOpacity(1);                      // ���õ���Ч��
    this->setWindowFlags(Qt::FramelessWindowHint);  // ������ʾ��ǰ ��Qt::WindowStaysOnTopHint
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground,true);
    this->setAcceptDrops(true);                           // ֧���Ϸ�

    codec = QTextCodec::codecForName("gb18030");    //  ���ò����б��ȡ�����ֱ���
    bSize = true;
    bLrcShow = true;                                // ���Ĭ�ϲ���ʾ
    bSwitchDesLrc = true;                           // ��������ʾ�Ŀ��Ʊ���
    bDesLrc = true;
    bDoubleClicked = false;                         // ˫����ʱ����ʾ��ʵĿ���

    currentRow = 0;
    DesLrcLabel = new lrcWindow(0);                 // ������
    DesLrcLabel->show();                            // hide at first

    initWidget();                                   // ��ʼ�� mediaObject �� audioObject��
    createLayout();                                 // ����
    createSystemTray();                             // ����ϵͳ����ͼ��ɶ��  ���˳��¼�ʱ�ŵ���
    setButtonDisenable();                           // ���ð�ť������
    readMusicList();                                // ���ļ��������б�
    doAnimation();                                  // �����Ķ���

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

    // Ҫ������audioOutput���������Ƶ���ղ�ֱ������Ƶ������ͨ�ŵĲ����ɲ��ֲ��䵱MediaObject��������Ƶ�豸��
    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory,this); //
    mediaObject = new Phonon::MediaObject(this);        //
    metaInfomationResolver  = new Phonon::MediaObject(this);    // ָ��ǰ��Ƶ��

    audioOutputPath = Phonon::createPath(mediaObject,audioOutput);    // ���ղۺ�ý�����֮�佨��������
    effectDescriptions = Phonon::BackendCapabilities::availableAudioEffects();
    audioEffect = NULL;

    // ���Զ���һ��������
    connect(mediaObject,SIGNAL(aboutToFinish()),this,SLOT(aboutToFinish()));
    // �벥�Ž���������  finished������   ʵ��ѭ������
    connect(mediaObject,SIGNAL(finished()),this,SLOT(finished()));
    // LCDNumber ÿ�������ʾ �� ��ȡ��ǰ������ʱ��   ���˽�вۺ���
    connect(mediaObject,SIGNAL(tick(qint64)),this,SLOT(tickAndTotalTime(qint64)));
    connect(mediaObject,SIGNAL(tick(qint64)),this,SLOT(showDesktopLRC(qint64))); // ������
    // ý��Ԫ״̬�ı�����
    connect(metaInfomationResolver,SIGNAL(stateChanged(Phonon::State,Phonon::State)),
            this, SLOT(metaStateChanged(Phonon::State, Phonon::State)));
    // ý��Դ�ı�����  ���˽�вۺ���  ʵ����Դ��ͬ���ı�
    connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
             this, SLOT(sourceChanged(const Phonon::MediaSource &)));

    mediaObject->setTickInterval(1000);        // ����1S�Ӹ���һ��ʱ��  

    /*  ********************* ����ؼ�  *************************** */
    // ������
    seekSlider = new Phonon::SeekSlider(this);    //  ������
    seekSlider->setMediaObject(mediaObject);

     // ������
    volumeSlider = new Phonon::VolumeSlider(this);
    volumeSlider->setAudioOutput(audioOutput);   //  ����豸      ������
    volumeSlider->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);  //  ������Χ

    musicTimeLabel = new QLabel(tr("00:00/00:00"),this);
    musicTimeLabel->setFont(QFont(tr("Times New Roman"),16));
    musicTimeLabel->show();

    // lrc ��ʰ�ť
    lrcBtn = new QToolButton(this);
    lrcBtn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    lrcBtn->setAutoRaise(true);
    lrcBtn->setAutoRepeat(true);
    lrcBtn->setToolTip(tr("��ʾ���"));
    lrcBtn->setIcon(QIcon(":/images/lrc.png"));
    lrcBtn->setEnabled(false);
    connect(lrcBtn,SIGNAL(clicked()), this, SLOT(showLRC()));

    // ��������
    lrcTextBrowser = new QTextBrowser;           // �����ʾ��
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

    // ���ڶԻ���
    aboutBtn = new QToolButton(this);
    aboutBtn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    aboutBtn->setAutoRaise(true);
    aboutBtn->setAutoRepeat(true);
    aboutBtn->setToolTip(tr("About The MusicPlayer"));
    aboutBtn->setIcon(QIcon(":/images/logo.png"));
    connect(aboutBtn,SIGNAL(clicked()), this, SLOT(about()));
    connect(aboutQtBtn, SIGNAL(clicked()),qApp,SLOT(aboutQt()));     // ����Qt

}


// create layout and connection
void MediaPlayer::createLayout()
{

    /***********************   �����б�    **************************/
   // QStringList headers;
    //headers << tr("Title");

    // �����б� tableWidget
    musicTable = new QTableWidget(0,1);
    musicTable->setAcceptDrops(false);      // ��֧���Ϸ�
    musicTable->setStyleSheet("QTableWidget {border-image: url(:/images/tablebg.png);"
                              "background-color:rgba(200,255,255,255);"
                              "selection-background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                              "stop:0 #eccad7, stop: 0.5 #f7b6cf,"
                             "stop: 0.6 #f9a2c3, stop:1 #FF92BB);}");

    musicTable->verticalHeader()->setStyleSheet("QHeaderView::section {background: rgb(207,206,222);"
                                                "padding-left: 4px;border: 1px solid #6c6c6c;}");
    musicTable->horizontalHeader()->hide();             // ˮƽ����������

    musicTable->horizontalHeader()->resizeSection(0,150);
    musicTable->setShowGrid(false);
    musicTable->setFont(QFont(tr("Times New Roman"),10));
    //musicTable->setHorizontalHeaderLabels(headers);     //  ���ñ���
    musicTable->setSelectionMode(QAbstractItemView::SingleSelection);   //  ��ѡ
    musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);    // ѡ����
    // ��������  ʵ��˫���б�ʱ��������
    connect(musicTable,SIGNAL(cellDoubleClicked(int,int)),
            this,SLOT(tableWidget_cellDoubleClicked(int,int)));


    /*************************  ���ֹ���    ******************************/
    QHBoxLayout *toolsBtnLayout = new QHBoxLayout;
    toolsBtnLayout->addStretch(20);
    toolsBtnLayout->addWidget(smallBtn);
    toolsBtnLayout->addWidget(expendBtn);
    toolsBtnLayout->addWidget(closeBtn);

    // ������ť����
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(openBtn);
    btnLayout->addWidget(playBtn);
    btnLayout->addWidget(prevBtn);
    btnLayout->addWidget(nextBtn);

    //  ������LCDʱ�䡢����
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(volumeSlider);
    hlayout->addWidget(musicTimeLabel);
    hlayout->addWidget(lrcBtn);

    // ������
    QHBoxLayout *seekSliderLayout = new QHBoxLayout;
    seekSliderLayout->addWidget(seekSlider);
    seekSliderLayout->addWidget(aboutBtn);

    /***********************   ���Ĳ���    *****************************/
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(toolsBtnLayout);
  //  mainLayout->addLayout(playing);
    mainLayout->addLayout(btnLayout);
    mainLayout->addLayout(seekSliderLayout);
    mainLayout->addLayout(hlayout);
    mainLayout->addWidget(musicTable);

    setLayout(mainLayout);

}

// �����¼�
void MediaPlayer::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    // ������Ⱦ  ����� �����������
    painter.setRenderHints(QPainter::Antialiasing|QPainter::HighQualityAntialiasing);
    painter.setBrush(QColor(212,212,212));
    painter.drawRoundedRect(rect(),10,10,Qt::AbsoluteSize);

}

// ���¼�
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

 // drop�¼�
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

     // �������б����������
     QTableWidgetItem *newItem = new QTableWidgetItem(musicFileName);
     int rowIndex = musicTable->rowCount();
     musicTable->insertRow(rowIndex);             // �²���һ��,�����item
     musicTable->setItem(rowIndex, 0, newItem);     // ���뵽�б���
     newItem->setFlags(newItem->flags() ^ Qt::ItemIsEditable);   //  �ɱ༭��

     // ��������Դsources���������
     Phonon::MediaSource musicItem(fileName);       // ����һ���µ�����
     sources.append( musicItem );                   //  ��ӵ������б�

     addFileToList(fileName);                       // д���ļ�

     setBackgroundRole(QPalette::NoRole);
     event->acceptProposedAction();
 }

 // �ϷŲ���,��������ӵ��ļ��б���
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
     ListStream << musicTitle<<"\n";     // ���ļ�����Ϣд�뵽�ļ�

     file.close();
 }

 void MediaPlayer::dragLeaveEvent(QDragLeaveEvent *event)
 {
     event->accept();
 }



 //�����¼�
void MediaPlayer::wheelEvent(QWheelEvent *wheelevent)
{
    if(wheelevent->delta() > 0 )    //�Ϲ�
    {
        qreal newVolume = audioOutput->volume() + (qreal)0.05;
         if(newVolume >= (qreal)1)
                newVolume = (qreal)1;
          audioOutput->setVolume(newVolume);
    }
    else    //�¹�
    {
            qreal newVolume = audioOutput->volume() - (qreal)0.05;
            if(newVolume <= (qreal)0)
                newVolume = (qreal)0;
            audioOutput->setVolume(newVolume);
    }

}

// ����ϵͳ����ͼƬ���˵�
void MediaPlayer::createSystemTray()
{

    showDesktopLRCAction = new QAction(QIcon(":/images/showlrc.png"),tr("����������"),this);
    connect(showDesktopLRCAction,SIGNAL(triggered()),this,SLOT(showDesLrc()));
    // play
    playAction = new QAction(QIcon(":/images/play.png"),tr("����"),this);
    connect(playAction,SIGNAL(triggered()),this,SLOT(on_playBtn_clicked()));
    //  next track
    nextTrackAction = new QAction(QIcon(":/images/next.png"),tr("��һ��"),this);
    connect(nextTrackAction,SIGNAL(triggered()),this,SLOT(on_nextBtn_clicked()));
    // previous track
    prevTrackAction = new QAction(QIcon(":/images/prev.png"),tr("��һ��"),this);
    connect(prevTrackAction,SIGNAL(triggered()),this,SLOT(on_prevBtn_clicked()));

    removeAction = new QAction(QIcon(":/images/remove.png"),tr("�Ƴ�����"),this);
   // removeAction->setEnabled(false);
    connect(removeAction,SIGNAL(triggered()),this,SLOT(removeCurrentMusic()));

    removeAllAction = new QAction(QIcon(":/images/clear.png"),tr("����б�"),this);
    connect(removeAllAction,SIGNAL(triggered()),this,SLOT(removeMusicList()));

    // ��ʾ
    ShowAction = new QAction(QIcon(":/images/logo.png"),tr("��ʾ������"),this);
    connect(ShowAction,SIGNAL(triggered()),this,SLOT(showWidget()));
    // ϵͳ����ͼ��
    hideTrayIconAction = new QAction(QIcon(":/images/collapse.png"),tr("���ز˵�"),this);
    connect(hideTrayIconAction,SIGNAL(triggered()),this,SLOT(hideTrayIconSlot()));

    // �˳�
    exitAction = new QAction(QIcon(":/images/exitlogo.png"),tr("�˳�"),this);
    connect(exitAction,SIGNAL(triggered()),this,SLOT(close()));

    /** *ϵͳ����ͼ��*  ***/
    trayicon = new QSystemTrayIcon(QIcon(":/images/logo.png"),this);
    // ��������  ���ĵ��� activated(QSystemTrayIcon::ActivationReason) is a signal,
    // after doubleClicked is show the widget,����signal��slot������һ�£�����˫���޷���ʾwidget
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

    trayicon->setToolTip(tr("������..."));
    trayicon->setContextMenu(trayiconMenu);

    trayicon->show();
    trayicon->showMessage(tr("MusicPlayer"),tr("Mr.Simple's MusicPlayer"),QSystemTrayIcon::Information,10000);

}

// �Ҽ��˵�
void MediaPlayer::contextMenuEvent(QContextMenuEvent *e)
{
    // ��ʾ�����˵� ʹ�ö����� QActionGroup
    QMenu *audioEffectMenu = new QMenu(tr("��Ч"),this);
    audioEffectMenu->setIcon(QIcon(":images/logo.png"));

    QActionGroup *audioEffectGroup = new QActionGroup(audioEffectMenu);   // ����aspectMenu�Ķ�����
    // ����QAction����ۺ���������
    connect(audioEffectGroup, SIGNAL(triggered(QAction*)), this,
                                        SLOT(audioEffectChanged(QAction*)));
    audioEffectGroup->setExclusive(true);

    // �����˵�
    Param_EQ = audioEffectMenu->addAction(tr("ParamEQ"));  // ��Ӷ������˵�
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

    // �Ҽ��˵���
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

// ���ð�ť������
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

// ���ð�ť����
void MediaPlayer::setButtonEnable()
{
    playBtn->setEnabled(true);         // ���Ű�ť������
    prevBtn->setEnabled(true);
    nextBtn->setEnabled(true);
    playAction->setEnabled(true);
    nextTrackAction->setEnabled(true);
    prevTrackAction->setEnabled(true);
    showDesktopLRCAction->setEnabled(true);
    lrcBtn->setEnabled(true);
}

// ��ȡ�����б�
void MediaPlayer::readMusicList()
{

    QFile srcFile("musicList.txt");
    if(!srcFile.open(QIODevice::ReadOnly)){
        return ;
    }
    int index = sources.size();

    QTextStream in(&srcFile);
    in.setCodec(codec);     // ���ñ���

    while(!in.atEnd()){     // û�ж����������ȡ   ���в���
        QString musicList = in.readLine();     // read all

        // ����������������Ϊý��Դ
        Phonon::MediaSource newSource(musicList);
        sources.append(newSource);              // ׷�ӵ����Ա����sources��
        metaInfomationResolver->setCurrentSource(sources.at(index));
        mediaObject->setCurrentSource(sources.at(index));

        if(musicList.length() > 5){
            setButtonEnable();          // ��ť����
        }

    }
    srcFile.close();
}

// ������
void MediaPlayer::showDesLrc()
{
    if(bSwitchDesLrc){
         DesLrcLabel->hide();
         showDesktopLRCAction->setText(tr("����������"));
         bSwitchDesLrc = false;
    }
    else{
        DesLrcLabel->show();
        showDesktopLRCAction->setText(tr("��ʾ������"));
        bSwitchDesLrc = true;
    }

}

// �Ƴ���ǰ�����б�
void MediaPlayer::removeCurrentMusic()
{
    int key = QMessageBox::warning(this, tr("��ʾ"),tr("ȷ���Ƴ���ǰ������?"),
                                   QMessageBox::Yes|QMessageBox::No);

    if(key == QMessageBox::Yes){                // �Ƴ�����

        QString title = musicTable->currentItem()->text();
        int currentRow = musicTable->currentRow();
        musicTable->removeRow(currentRow);
        sources.removeAt(currentRow);           // �Ƴ����

        QList<QString> musicList;               // �洢�����б�
        readMusicListOnly(musicList, title);    // ����������QList<QString>��
        writeMusicListOnly(musicList);          // ��д�����б��ļ���
    }
    else
    {
        return ;
    }

}

// ֻ���б�һ��QList<QString>��
void MediaPlayer::readMusicListOnly(QList<QString>& musicList, QString title)
{

    QFile file("musicList.txt");
    if(!file.open(QIODevice::ReadOnly)){
        return ;
    }
    QTextStream in(&file);
    in.setCodec(codec);                 // ���ñ���

    while(!in.atEnd()){                 // û�ж����������ȡ   ���в���
        QString temp = in.readLine();
        if(temp.contains(title))        // ����ѡ�еĸ���
        {
            continue;
        }
        musicList<<temp<<"\n";
    }
    file.close();
}

// ֻ�������б�д���ļ���
void MediaPlayer::writeMusicListOnly(const QList<QString> musicList)

{
    QFile file("musicList.txt");
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        return ;
    }

    QTextStream ListStream(&file);
    foreach(QString musicTitle, musicList){

        /***************  ���沥���б�  *****************/
        ListStream.setAutoDetectUnicode(false);
        ListStream.setCodec(codec);
        // ���б������musicList.txt ����  musicListString��ֻ�Ǳ������ļ�·�����ļ���
        ListStream << musicTitle;     // ���ļ�����Ϣ������ļ�
    }
    file.close();
}


// ��ղ����б�
void MediaPlayer::removeMusicList()//����б�
{
    mediaObject->pause();
    QList<Phonon::MediaSource> emptySource;
    sources = emptySource;   // �ÿ�ý����

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
    playBtn->setIconSize(QSize(48,48));     // ����ICON�Ĵ�С
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

// ��Ч����
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

// �б��е���Դ�ı�
void MediaPlayer::sourceChanged(const Phonon::MediaSource &source)
{
    musicTable->selectRow(sources.indexOf(source));
    musicTimeLabel->setText(tr("00:00/00:00"));
}

// ��ý��״̬�ı� �ο�qt����
void MediaPlayer::metaStateChanged(Phonon::State newState, Phonon::State oldState)
{
    if(newState == Phonon::ErrorState)
      {
          QMessageBox::warning(this,tr("Error open file"),
                               metaInfomationResolver->errorString());
          while(!sources.isEmpty() &&
              !(sources.takeLast() == metaInfomationResolver->currentSource()));
                    {} ;    //  ���ݲ�Ϊ�պͲ����������ʱ   ѭ��   �����ִ��
              return ;
      }

    //  ��ͣ��ֹͣ״̬��ֱ�ӷ���
    if(newState != Phonon::StoppedState && newState != Phonon::PausedState)
        return ;

    //  ����ý������Ч��  ֱ�ӷ���
    if(metaInfomationResolver->currentSource().type() == Phonon::MediaSource::Invalid)
        return ;

    //  ��ȡ����Դ   Returns the strings associated with the given key
    QMap<QString,QString> metaData = metaInfomationResolver->metaData();

   // musicFileName = metaData.value("TITLE");   //  ������  һ��mp3��ʽ������  apeһ����

    QString fileName = metaInfomationResolver->currentSource().fileName();
        // ȥ��·�� �����ȡ���ļ���Ϊ "E:/Musics/֪��.mp3"  ��ִ�к�Ϊ "֪��.mp3"
    musicFileName = fileName.right(fileName.length() - fileName.lastIndexOf('/') - 1);
    musicFilePath = fileName.left(fileName.length() - musicFileName.length());

    //  ���ֱ�
    QTableWidgetItem *titleItem = new QTableWidgetItem(musicFileName);
    titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);   //  �ɱ༭��
    QTableWidgetItem *artistItem = new QTableWidgetItem(metaData.value("ARTIST"));
    artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);

    //  ����ý���б�  ���ֱ�
    currentRow = musicTable->rowCount();
    musicTable->insertRow(currentRow);
    musicTable->setItem(currentRow, 0, titleItem);

    if (musicTable->selectedItems().isEmpty()) {
        musicTable->selectRow(0);    // ѡ���һ�е�����Դ
        mediaObject->setCurrentSource(metaInfomationResolver->currentSource());
    }

    Phonon::MediaSource source = metaInfomationResolver->currentSource();
    int index = sources.indexOf(metaInfomationResolver->currentSource()) + 1; // ѡ�������+1 ָ������һ��

    if (sources.size() > index) {     //  �ļ�Դ�Ĵ�С����   ������ý��Դ������Ϊindex
        metaInfomationResolver->setCurrentSource(sources.at(index));
    }
    else {
        musicTable->resizeColumnsToContents();
        if (musicTable->columnWidth(0) > 400)
                musicTable->setColumnWidth(0, 400);   // �������еĿ��
    }

    lrcHash.clear();       // �����ʵ�����
    lrcHash.squeeze();
    bDesLrc = true;
}


// �ر��¼�  ��С����ϵͳ����   ��С����ť�Ĳ�
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
                           this->setWindowState(Qt::WindowActive);  //�ָ�������ʾ
                           this->repaint();
                           e->ignore();
            default:
                   break;
        }
}

// ˫��ϵͳ������ʾ���Ŵ���   ���˵��� �������м����
void MediaPlayer::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::Trigger://����
    case QSystemTrayIcon::MiddleClick:
        break;
    case QSystemTrayIcon::DoubleClick://˫��
        this->setWindowState(Qt::WindowActive);//�ָ�������ʾ
        this->show();
        if(!bLrcShow){
            lrcTextBrowser->show();  // ��ʴ�����ʾ
        }
        break;
    default:
        break;
    }
}

// ʱ����ʾ�ۺ��� LCDNumber��ʾ �� ������ʱ����ʾ   LCDNumberÿ�����
void MediaPlayer::tickAndTotalTime(qint64 time)
{
    // ��ǰ��������ʱ��
    qint64 tempTime = mediaObject->totalTime();
    QTime totalTime(0,(tempTime/60000)%60,(tempTime/1000)%60,tempTime%1000);    // ��ʱ��
    QTime curTime(0,(time / 60000) % 60,(time / 1000) % 60,time %1000);    //  ��ǰʱ�� ��tick(qint64) ʱ��ͬ��
    musicTimeLabel->setText(tr("%1/%2").arg(curTime.toString("mm:ss")).arg(totalTime.toString("mm:ss")));
    musicTimeLabel->update();
}

/* ************ ���ޱ����������  ��갴�º��ƶ�����������ʵ�ִ��ڵ��ƶ� *********** */
void MediaPlayer::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->globalPos();       // ����ԭʼλ��
    newPos = event->globalPos() - event->pos();         // ��ǰ���λ��

}

void MediaPlayer::mouseMoveEvent(QMouseEvent *event)
{
    QPoint newPos0 = event->globalPos();
    QPoint finalPos = newPos + newPos0 - lastPos;
    move(finalPos);     // �����ƶ�����
}
/*  ******************************* ���ڵ��ƶ�  ******************************/


/*  ************************** Animation functions  ���� **********************/
// ��������
void MediaPlayer::doAnimation()
{
        QPropertyAnimation *amt = new QPropertyAnimation(this,"geometry",this);
        amt->setDuration(2500);
        QRect originalRect = this->geometry();      // ԭʼ�����С

        amt->setStartValue(originalRect);
        amt->setEndValue(QRect(950,40,280,400));
        amt->setEasingCurve(QEasingCurve::OutExpo);
        amt->start();
}

// ���ڵ���С����չ
void MediaPlayer::windowExpand()
{
     QPropertyAnimation *amt = new QPropertyAnimation(this,"geometry",this);
     amt->setDuration(1400);
     QRect originalRect = this->geometry();
     amt->setStartValue(originalRect);

     if(bSize){     // ��С
            amt->setEndValue(QRect(950,40,280,120));
            expendBtn->setToolTip(tr("Expand"));
            expendBtn->setIcon(QIcon(":/images/collapse.png"));
            expendBtn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            expendBtn->setIconSize(QSize(18,18));
            bSize = false;
            amt->setEasingCurve(QEasingCurve::InOutBack);   // ������Ч
            amt->start();      // start the animation
            musicTable->hide();
     }
     else{      // ��չ
         amt->setEndValue(QRect(950,40,280,400));
         expendBtn->setToolTip(tr("Collapse"));
         expendBtn->setIcon(QIcon(":/images/expand.png"));
         expendBtn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
         expendBtn->setIconSize(QSize(20,20));
         bSize = true;
         amt->setEasingCurve(QEasingCurve::OutBounce);      // ������Ч
         amt->start();      // start the animation
         musicTable->show();
     }

}
/*  **************************** Animation functions  end **************************/


// ����ļ���ť
void MediaPlayer::addFiles()
{
   QStringList files = QFileDialog::getOpenFileNames(this,tr("Open music files"),
                                                     QDesktopServices::storageLocation(QDesktopServices::MusicLocation),
                                                     tr("Music (*.mp3 *.wma *.ape *.wav)"));
    playBtn->setChecked(false);

    if(files.isEmpty())     // Ϊ��  �򷵻�
        return ;

    // �����б��ļ�,���ڱ��沥���б���
    QFile fileList("musicList.txt");

    if(!fileList.open(QIODevice::WriteOnly | QIODevice::Append)){
        return ;
    }

    QTextStream ListStream(&fileList);

    int index = sources.size();     // ���˽�г�Ա���� sources �洢��ý����Դ��Ϣ

    foreach(QString fileName,files){

        Phonon::MediaSource NewSources(fileName);  // ��ǰ��ӵ���Դ
        sources.append(NewSources);             //  ��NewSources���������ӵ�sources����
        fileName.append("\n");

        /***************  ���沥���б�  *****************/
        ListStream.setAutoDetectUnicode(false);
        ListStream.setCodec(codec);
        // ���б������musicList.txt ����  musicListString��ֻ�Ǳ������ļ�·�����ļ���
        ListStream << fileName;     // ���ļ�����Ϣ������ļ�
    }

    if(!sources.isEmpty()){             //  ��Դ��Ϊ��
        mediaObject->stop();
        metaInfomationResolver->setCurrentSource(sources.at(index));   // ָ��ǰ��Դ��ָ��
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
    fileList.close();       // �ر��ļ�
}

// ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�
void MediaPlayer::getMusicPathAndName()
{
    /* ***************** ���»�ȡ�������ƺ�·��  �Ա���ʾ��� ******************/
    QString fileName = mediaObject->currentSource().fileName();
    musicFileName = "";
    musicFilePath = "";
    musicFileName = fileName.right(fileName.length() - fileName.lastIndexOf('/') - 1);  // �����ļ���
    musicFilePath = fileName.left(fileName.length() - musicFileName.length());      // ·����
    QString showName = musicFileName.left(musicFileName.length() - 4);  // ��ø���ʵ��
    showLabel->setText("Playing: " + showName);
}

// ˫�������б���
void MediaPlayer::tableWidget_cellDoubleClicked(int row, int column)//˫��ѡ��
{
    playBtn->setIcon(QIcon(":/images/push.png"));   // ��ͣ
    mediaObject->stop();
    mediaObject->clearQueue();      // ��ն���

    if (row >= sources.size())
        return;

    mediaObject->setCurrentSource(sources[row]);   // QStringList������[]��  ѡ��ǰ��Ŀ
    mediaObject->play();    // ����

    getMusicPathAndName();    // ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�

    // ˫����ʱ�� ��ո��
    lrcTextBrowser->clear();
    if(bDoubleClicked){       // ������Ѿ���ʾ��ʱ����˫��  ����ø����ʾ
        bLrcShow = true;   // ��ʾ�Ժ�����  bLrcShowΪtrue  �ٰ�һ�¸�ʰ�ť ��Ϊ����
        showLRC(); 
    }
    lrcHash.clear();
    lrcHash.squeeze();
     bDesLrc = true;

}

//  ���š���ͣ����
void MediaPlayer::playOrPause()
{
    playAction->setIcon(QIcon(":/images/push.png"));
    playAction->setText(tr("��ͣ"));

    switch(mediaObject->state()){

    case Phonon::PlayingState:
        playBtn->setIcon(QIcon(":/images/play.png"));
        playBtn->setIconSize(QSize(48,48));     // ����ICON�Ĵ�С
        playBtn->setToolTip(tr("Play"));
        playAction->setIcon(QIcon(":/images/play.png"));
        playAction->setText(tr("����"));
        mediaObject->pause();
        playBtn->setChecked(false);
        break;
    case Phonon::PausedState:
        playBtn->setIcon(QIcon(":/images/push.png"));   // ��ͣ
        playBtn->setToolTip(tr("Pause"));
        playAction->setIcon(QIcon(":/images/push.png"));
        playAction->setText(tr("��ͣ"));
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

   getMusicPathAndName();   // ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�
}

// next  track
void MediaPlayer::nextMusic()
{
    // ��ǰ��Դ������1 ����һ������λ��
    int index = sources.indexOf(mediaObject->currentSource()) + 1;

    if(sources.size() > index){
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(index));        // ���õ�ǰ��Դ�ļ�����
        mediaObject->play();                                 // ����
        musicTable->selectRow(index);
    }
    else{       // ��ʱΪ���һ��  ��ת����һ��
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(0));        // ���õ�ǰ��Դ�ļ�����
        mediaObject->play();
    }

    getMusicPathAndName();   // ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�
    if(!bLrcShow){        // �Ѿ���ʾ��ʴ��ڵ������
         bLrcShow = true;
         showLRC();
    }


}

// previous track
void MediaPlayer::preMusic()
{
    int index = sources.indexOf(mediaObject->currentSource()) - 1;    // ��ȡ��һ������λ��
    if(index < 0){      // ��ʱָ���˵�һ��  �ٵ���һ�����������һ��
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(currentRow));    // ����Ϊ���һ��
        mediaObject->play();
        musicTable->selectRow(index);
    }
    else{
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(index));        // ���õ�ǰ��Դ�ļ�����
        mediaObject->play();            // ����
    }

  getMusicPathAndName();   // ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�
  if(!bLrcShow){        // �Ѿ���ʾ��ʴ��ڵ������
       bLrcShow = true;
       showLRC();
  }
}

// about to finish current track,�Զ�������һ��
void MediaPlayer::aboutToFinish()
{
    int index = sources.indexOf(mediaObject->currentSource()) + 1;       // ��ȡ��һ������λ��
    if(sources.size() > index){
        mediaObject->enqueue(sources.at(index));    // ��Ӳ���  ������Ŀʱ������һ�� ����ֹͣ
        mediaObject->stop();
        mediaObject->setCurrentSource(sources.at(index));
        mediaObject->play();     // ����
    }
    else
        playBtn->setChecked(false);

    getMusicPathAndName();   // ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�
    if(!bLrcShow){        // �Ѿ���ʾ��ʴ��ڵ������
         bLrcShow = true;
         showLRC();
    }

    lrcHash.clear();       // �����ʵ�����
    bDesLrc = true;
    showDesktopLRC(0);

}

// finished  �����ļ�Դ�Ľ�β
void MediaPlayer::finished()        //  ������Ϻ� ��ͷ��ʼѭ������
{
    this->show();
    this->setWindowState(Qt::WindowActive);//�ָ�������ʾ

    mediaObject->stop();
    mediaObject->setCurrentSource(sources.at(0));        // ���õ�ǰ��Դ�ļ�����
    mediaObject->play();

    getMusicPathAndName();   // ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�
    if(!bLrcShow){        // �Ѿ���ʾ��ʴ��ڵ������
         bLrcShow = true;
         showLRC();
    }

}


/*   *********************   signals and slots   ******************************  */

void MediaPlayer::showLRC()
{
    QRect rect = this->geometry();
    lrcTextBrowser->setGeometry(rect.x()-300,rect.y()+100,rect.width(),rect.height());

    QFile file;     // �ļ�
    QDir::setCurrent(musicFilePath);        // ���������ļ�·��Ϊ��ǰ·��
    QString lrcFileName(musicFileName);     // �����ļ���Ϊ���ڲ��ŵ��ļ���

    /*  ***********************************************************************
        �� .mp3�ļ����ĳ� .lrc  ������QRegExp�о����� һ��ֻ��ȥ��һ�ָ�ʽ��
        lrcFileName = lrcFileName.replace(QRegExp(".mp3"),".lrc");
        �� lrcFileName = lrcFileName.remove(musicFileName.right(3)) + "lrc";  ȫ����ʽ������
    *****************************************************************************/

     lrcFileName = lrcFileName.remove(musicFileName.right(3)) + "lrc";
     file.setFileName(lrcFileName);

    QString lrcCache;        // ���ļ��ж����������ݴ������

    if(bLrcShow){      // ��ťʵ����ʾ��ʺ����ع���

        QString lrc;
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)) // �Ѷϸ���ļ��Ƿ��
        {
           QTextStream lrcStream(&file);
           QTextCodec *codec = QTextCodec::codecForName("gb18030"); //�����������
           lrcStream.setCodec(codec);

           while(!(lrcStream.atEnd())){         // û�ж����ʱ��
              lrcCache = lrcStream.readLine();   // ��ȡÿһ�и���ļ���ʱ�� �� ��ʲ���
              int numTimes = lrcCache.count(']');  // ��ȡ���и�����м���  ]
               // ��ȡʱ��  �� QMap������� [00.16.23] ���ȡ��  00.16.23
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
           lrcTextBrowser->show();           // ��ʾ���
           bLrcShow = false;        // �Ѿ���ʾ������Ϊfalse
           bDoubleClicked = true;
           lrcBtn->setToolTip(tr("���ظ��"));
        }
        else{
             QSound::play("warning.wav");
             lrcTextBrowser->hide();
             lrcTextBrowser->clear();
             bLrcShow = true;
             bDoubleClicked = false;        // ˫���Ƿ���ʾ������
             lrcBtn->setToolTip(tr("��ʾ���"));
             QMessageBox::warning(this,tr("Warning"),
                                  tr("%1 û�и���ļ�,���ֶ�����!").arg(musicFileName),
                                  QMessageBox::Yes);
            }
    }
    else{
        lrcTextBrowser->clear();
        lrcTextBrowser->hide();
        bLrcShow = true;
        bDoubleClicked = false;
        lrcBtn->setToolTip(tr("��ʾ���"));
    }

    file.close();       // close the lrc file
}


// ������
void MediaPlayer::showDesktopLRC(qint64 time)
{
    getMusicPathAndName();
 /*   bool bTwo = false;
    bool bThr = false;
    */

    if(lrcHash.isEmpty()){   // ���˸����Ļ���Ҫ�ض��ļ�
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

    QFile file;  // ����ļ�

    if(bDesLrc){   // ��Ҫ�ض��ļ� bDesLrc�ǿ��Ʊ���
        QDir::setCurrent(musicFilePath);        // ���������ļ�·��Ϊ��ǰ·��
        QString lrcFileName(musicFileName);     // �����ļ���Ϊ���ڲ��ŵ��ļ���
        lrcFileName = lrcFileName.remove(musicFileName.right(3)) + "lrc";
        file.setFileName(lrcFileName);
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)) // �Ѷϸ���ļ��Ƿ��
            bDesLrc = false;

        }

        QString lrcCache = "";        // ���ļ��ж����������ݴ������   ����

        QTextStream lrcStream(&file);
        QTextCodec *codec = QTextCodec::codecForName("gb18030"); //�����������
        lrcStream.setCodec(codec);
        // read the lrc stream
        while(!(lrcStream.atEnd())){       // û�ж����ʱ��

            int timeTwo = 0,timeThr = 0;
            bool bTwo = false;
            bool bThr = false;

            lrcCache = lrcStream.readLine();    // read line          
            if(lrcCache == "")  // the line is empty,skip
                  continue ;
            if(lrcCache.endsWith("]"))  // the line is end with "]" ,skip
                    continue ;

            int temp = lrcCache.mid(2,1).toInt();   // [00:45.34] ����ӱ�ת��Ϊint Ҳ�� 0
            int iOne = lrcCache.mid(4,2).toInt();
            int itime = temp*60 + iOne;

            int numTimes = lrcCache.count(']');  // ��ȡ���и�����м���']'
            if(numTimes == 2){    // [02:48.01][01:10.00]û�뵽ʧȥ�������һ�����
                      bTwo = true;
                      int itwo = lrcCache.mid(12,1).toInt();
                      timeTwo = lrcCache.mid(14,2).toInt() + itwo*60;
                  }
            if(numTimes == 3){    // [03:40.00][03:02.00][01:30.00]������һ�ж��ݵ�����
                      bTwo = true;
                      bThr = true;
                      int itwo = lrcCache.mid(12,1).toInt();
                      timeTwo = lrcCache.mid(14,2).toInt() + itwo*60;
                      int ithr = lrcCache.mid(22,1).toInt();
                      timeThr = lrcCache.mid(24,2).toInt() + ithr*60;
                  }

            if(bThr)    // ��������ʱ�� ȥ��ǰ����ʮ���ַ�
               lrcCache.replace(0,30,"");
            else if(bTwo)   // ��������ʱ����
                     lrcCache.replace(0,20,"");  // �滻��  replace

            // ȥ�����ǰ���ʱ�� ���� [00:23.11]hello ��Ϊ hello
            lrcCache.replace(QRegExp("\\[\\d{2}:\\d{2}\\.\\d{2}\\]"),""); // [03:40.00] ����ʽȥ����
            lrcHash.insert(itime, lrcCache);    // ����QHash��
            if(bTwo)    // ������ʱ��
                lrcHash.insert(timeTwo, lrcCache);
            if(bThr)    // ����ʱ��
                lrcHash.insert(timeThr, lrcCache);

        }  // ��ȡ��ʽ���

        file.close();    // close the file

        // ������ϣ��  �ҳ���ʱ���߶�Ӧ�ĸ��
        QHash<int,QString>::const_iterator iterator = lrcHash.constBegin();
        while (iterator != lrcHash.constEnd()) {
             if((iterator.key()) == realTime){  // ��ǰʱ�����ϣ���ӵ�ʱ�����  �����������
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
    getMusicPathAndName();   // ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�
    lrcHash.clear();
    lrcHash.squeeze();
    bDesLrc = true;

}

void MediaPlayer::on_nextBtn_clicked()
{
    nextMusic();
    getMusicPathAndName();   // ��ȡ�����ļ�����·��  �Ա���Ҹ���ļ�
    lrcHash.clear();       // �����ʵ�����
    lrcHash.squeeze();
    bDesLrc = true;
}

void MediaPlayer::on_closeBtn_clicked()
{
    close();        // ������  closeEvent
}

void MediaPlayer::on_expendBtn_clicked()
{
   windowExpand();
}

void MediaPlayer::on_smallBtn_clicked()
{

    this->setWindowState(Qt::WindowMinimized);
    if(!bLrcShow){
        lrcTextBrowser->hide();        // ���ظ��
    }

}

void MediaPlayer::hideTrayIconSlot()
{
    trayiconMenu->hide();
}

void MediaPlayer::showWidget()
{
    this->setWindowState(Qt::WindowActive);//�ָ�������ʾ
    this->show();
    if(!bLrcShow){        // �Ѿ���ʾ��ʴ��ڵ������
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
