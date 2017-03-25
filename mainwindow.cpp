#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDebug>
#include <QDragEnterEvent>
#include <QSettings>
#include <QDateTime>
#include <QtXml>
#include <ctime>
#include <stdlib.h>
#include <new>

#define VERSION "1.0.27"

/*
 * TODO
 * music base w konfigu
 * menu kontekstowe pliku
 * *
 * menu kontekstowe playlisty
 * *usuń nie istniejące - zmienia się tytuł kolumny czas na 0 - popraw
 * menu kontekstowe kolejki odtwarzania
 * *
 * prążki dodaj!
 * przycisk odświerzania drzewa
 *
 * przenieść wczytywanie do drugiego wątku (może nie koniecznie)
 */

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_dir_model(nullptr),
    m_current_tree_model(nullptr),
    m_music_base_path("/media/Inne/Muzyka/"),
    m_current_play_file(nullptr),
    m_is_playing(false),
    m_moved_slider(false),
    m_track_finished(false),
    m_current_stream_handle(0),
    m_volume_level(1.0),
    m_duration(0.0),
    m_order(Order::Random),
    m_is_mute(false),
    m_tree_column_count(3),
    m_continue_playing(false),
    m_current_file_name(nullptr)
{
    ui->setupUi(this);

    m_pb_play = ui->pb_play;
    m_pb_pause = ui->pb_pause;
    m_pb_stop = ui->pb_stop;
    m_pb_next = ui->pb_next;
    m_pb_previous = ui->pb_previous;
    m_tv_music_base = ui->tv_music_base;
    m_le_music_base_path = ui->le_music_base_path;
    m_tb_set_music_base_path = ui->tb_set_music_base_path;
    m_hs_track_position = ui->hs_track_position;
    m_hs_volume_position = ui->hs_volume_position;
    m_l_title = ui->l_title;
    m_l_time_count_duration = ui->l_time_count_duration;
    m_tw_play_lists = ui->tw_play_lists;
    m_tb_play_lists = m_tw_play_lists->tabBar();
    m_pb_volume = ui->pb_volume;
    m_pb_simple_order = ui->pb_simple_order;
    m_pb_random_order = ui->pb_random_order;
    m_lv_play_queue = ui->xlv_play_queue;

    tree_headers << "Ścieżka" << "Nazwa" << "Czas" << "Odtworzenia" << "Istnieje";

    m_play_queue_model = new MyListModel(tree_headers);
    m_tb_play_lists->setContextMenuPolicy(Qt::CustomContextMenu);

    m_play_queue_model->setColumnCount(m_tree_column_count);
    m_play_queue_model->deleteEnable(true);
    m_lv_play_queue->setDragEnabled(true);
    m_lv_play_queue->setAcceptDrops(true);
    m_lv_play_queue->setModel(m_play_queue_model);

    m_update_track_position = new QTimer(this);
    m_update_track_position->setInterval(500);

    //Base
    connect(m_tb_set_music_base_path, SIGNAL(clicked()), this, SLOT(setMusicBase()));
    connect(m_le_music_base_path, SIGNAL(returnPressed()), this, SLOT(pathEditingFinish()));

    //Buttons
    connect(m_pb_play, SIGNAL(clicked()), this, SLOT(onPlayButton()));
    connect(m_pb_pause, SIGNAL(clicked()), this, SLOT(onPauseButton()));
    connect(m_pb_stop, SIGNAL(clicked()), this, SLOT(onStopButton()));
    connect(m_pb_next, SIGNAL(clicked()), this, SLOT(onNextButton()));
    connect(m_pb_previous, SIGNAL(clicked()), this, SLOT(onPreviousButton()));
    connect(m_pb_volume, SIGNAL(clicked()), this, SLOT(onVolumeButton()));
    connect(m_pb_simple_order, SIGNAL(clicked()), this, SLOT(onPlaybackOrder()));
    connect(m_pb_random_order, SIGNAL(clicked()), this, SLOT(onPlaybackOrder()));

    //Position Slider
    connect(m_hs_track_position, SIGNAL(sliderMoved(int)), this, SLOT(onSliderPositionChange(int)));
    connect(m_hs_track_position, SIGNAL(sliderPressed()), this, SLOT(onSliderPress()));
    connect(m_hs_track_position, SIGNAL(sliderReleased()), this, SLOT(onSliderRelease()));
    connect(m_update_track_position, SIGNAL(timeout()), this, SLOT(onMusicPositionChange()));

    //Play Lists
    connect(m_tw_play_lists, SIGNAL(currentChanged(int)), this, SLOT(onCurrentTabChanged(int)));
    connect(m_tb_play_lists, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onTabBarContextMenu(QPoint)));
    connect(ui->pb_add_play_list, SIGNAL(clicked()), this, SLOT(onAddPlayListClicked()));

    //Play Queue
    connect(m_lv_play_queue, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(fileActive(QModelIndex)));
    connect(m_lv_play_queue, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onPlayQueueContexttMenu(QPoint)));

    //Volume
    connect(m_hs_volume_position, SIGNAL(valueChanged(int)), this, SLOT(onVolumePositionChange(int)));

    m_tv_music_base->setDragEnabled(true);
    m_l_title->setTextFormat(Qt::RichText);

    m_tb_play_lists->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->splitter1->setOrientation(Qt::Horizontal);
    ui->splitter2->setOrientation(Qt::Vertical);

    m_hs_volume_position->setMaximum(100);
    m_hs_volume_position->setValue(100);

    ui->splitter2->setStretchFactor(0, ui->splitter2->height()/4*3);
    ui->splitter2->setStretchFactor(1, ui->splitter2->height()/4);

    // initialize BASS
    m_successfully_initialized = BASS_Init(-1,44100,0, NULL,NULL);  //BASS_Init(device(-1=default, sample rate, flags, parent window, Class to initialize DirectSound (NULL = default));


    loadAppConfig();


    m_le_music_base_path->setText(m_music_base_path);

    setDirModel();
    loadPlayLists();
    setShortcuts();
    setCurrentFile(nullptr);
    setVolumeLevel();

    QStringList args = qApp->arguments();
    if(args.size() > 1)
    {
        onAddPlayListClicked();
        QList<QString>::iterator iter = args.begin();
        iter++;
        for(;iter != args.end(); iter++)
        {
            m_current_tree_model->appendNewTree(*iter, true);
        }
    }

    m_search_dialog = new SearchDialog(this);
    connect(m_search_dialog, SIGNAL(activeFile(TreeItem*,bool)), this, SLOT(onSearchDialogActiveFile(TreeItem*,bool)));
    srand(time(NULL));
    ui->statusBar->showMessage(QString("Wersja %1").arg(VERSION));
}

MainWindow::~MainWindow()
{
    m_current_play_file = nullptr;

    onStopButton();
    BASS_Free();

    saveAppConfig();
    savePlayLists();

    if(m_search_dialog != nullptr)
    {
        m_search_dialog->close();
        delete m_search_dialog;
    }

    if(m_current_file_name != nullptr)
    {
        delete m_current_file_name;
        m_current_file_name = nullptr;
    }

    delete m_play_queue_model;
    delete m_dir_model;
    delete m_update_track_position;

    delete ui;
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    static int i = 0;
    switch(e->key())
    {
    case Qt::Key_Right:
        m_moved_slider = true;
        if(m_is_playing)
        {
            onPauseButton();
            m_continue_playing = true;
        }
        else
            onPauseButton();

        m_hs_track_position->setSliderPosition(m_hs_track_position->sliderPosition() + 1000);
        onSliderPositionChange(m_hs_track_position->sliderPosition());

        m_moved_slider = false;
        if(m_is_playing)
            onPlayButton();
        break;
    case Qt::Key_Left:
        m_moved_slider = true;
        if(m_is_playing)
        {
            onPauseButton();
            m_continue_playing = true;
        }
        else
            onPauseButton();

        m_hs_track_position->setSliderPosition(m_hs_track_position->sliderPosition() - 1000);
        onSliderPositionChange(m_hs_track_position->sliderPosition());

        m_moved_slider = false;
        if(m_is_playing)
            onPlayButton();
        break;
    }
}

void MainWindow::setDirModel()
{
    QStringList headers;
    headers << "Full Path" << "Nazwa" << "Ostatnia modyfikacja";

    if(m_dir_model) delete m_dir_model;
    m_dir_model = new MyDirModel(headers, QStringList() << m_music_base_path);
    m_dir_model->setColumnCount(1);
    m_dir_model->deleteEnable(true);
    m_tv_music_base->setModel(m_dir_model);
}

bool MainWindow::getNextFile()
{
    QFileInfo file_info;
    TreeItem* t;
    while(!m_play_queue_model->getTrees().isEmpty())
    {
        t = m_play_queue_model->getTrees().at(0);
        file_info.setFile(t->data(0).toString());
        if(t != m_current_play_file && file_info.exists())
            if(file_info.isFile())
            {
                setCurrentFile(t, true);
                return true;
            }
        m_play_queue_model->removeRows(0, 1);
    }

    if(m_current_tree_model == nullptr) return false;
    m_current_play_file = m_current_tree_model->getCurrentItem();

    if(m_order == Order::Random)
    {
        QList<TreeItem*> all = m_current_tree_model->getAllTrees();
        if(all.isEmpty())
        {
            setCurrentFile(nullptr);
            return false;
        }
        TreeItem* tmp;
        TreeItem* tmp2;
        QFileInfo info;
        all.removeOne(m_current_play_file);
        while(!all.isEmpty())
        {
            tmp = all.at(rand() % all.size());
            info.setFile(tmp->data(0).toString());

            if(!info.exists())
            {
                tmp->setData(4, false);
                all.removeOne(tmp);
            }
            else if(info.isDir())
            {
                all.removeOne(tmp);
            }
            else
            {
                all.removeOne(tmp);
                while(!all.isEmpty())
                {
                    tmp2 = all.at(rand() % all.size());
                    info.setFile(tmp->data(0).toString());

                    if(!info.exists())
                    {
                        tmp2->setData(4, false);
                        all.removeOne(tmp2);
                    }
                    else if(info.isDir())
                    {
                        all.removeOne(tmp2);
                    }
                    else
                    {
                        //jeśli mamy dwa utwory to wybieramy ten o większej liczbie odtworzeń z szansą 0.75
                        float p = static_cast<float>(rand()) / RAND_MAX;
                        if(tmp->data(3).toInt() > tmp2->data(3).toInt())
                            setCurrentFile((p < 0.65) ? tmp : tmp2);
                        else
                            setCurrentFile((p < 0.65) ? tmp2 : tmp);
                        return true;
                    }
                }
                setCurrentFile(tmp);
                return true;
            }
        }

        setCurrentFile(nullptr);
        return false;
    }
    else
    {
        QList<TreeItem*> all = m_current_tree_model->getAllTrees();
        if(all.isEmpty())
        {
            setCurrentFile(nullptr);
            return false;
        }
        QList<TreeItem*>::iterator iter;
        QFileInfo info;

        for(iter = all.begin(); iter != all.end(); iter++)
        {
            if((*iter) == m_current_play_file)
                break;
        }

        if(iter == all.end())
            iter--;
        bool exists;
        do
        {
            iter++;
            if(iter == all.end())
                iter = all.begin();

            if((*iter) == m_current_play_file)
            {
                setCurrentFile(nullptr);
                return false;
            }
            info.setFile((*iter)->data(0).toString());
            exists = info.exists();
            if(!exists) (*iter)->setData(4, false);
        }while(!exists || info.isDir());

        setCurrentFile(*iter);
        return true;
    }
    return false;
}

bool MainWindow::getPreviousFile()
{
    if(m_current_tree_model == nullptr) return false;
    m_current_play_file = m_current_tree_model->getCurrentItem();

    if(m_order == Order::Random)
    {
        QList<TreeItem*> all = m_current_tree_model->getAllTrees();
        if(all.isEmpty())
        {
            setCurrentFile(nullptr);
            return false;
        }
        TreeItem* tmp;
        TreeItem* tmp2;
        QFileInfo info;
        all.removeOne(m_current_play_file);
        while(!all.isEmpty())
        {
            tmp = all.at(rand() % all.size());
            info.setFile(tmp->data(0).toString());

            if(!info.exists())
            {
                tmp->setData(4, false);
                all.removeOne(tmp);
            }
            else if(info.isDir())
            {
                all.removeOne(tmp);
            }
            else
            {
                all.removeOne(tmp);
                while(!all.isEmpty())
                {
                    tmp2 = all.at(rand() % all.size());
                    info.setFile(tmp->data(0).toString());

                    if(!info.exists())
                    {
                        tmp2->setData(4, false);
                        all.removeOne(tmp2);
                    }
                    else if(info.isDir())
                    {
                        all.removeOne(tmp2);
                    }
                    else
                    {
                        //jeśli mamy dwa utwory to wybieramy ten o większej liczbie odtworzeń z szansą 0.75
                        float p = static_cast<float>(rand()) / RAND_MAX;
                        if(tmp->data(3).toInt() > tmp2->data(3).toInt())
                            setCurrentFile((p < 0.65) ? tmp : tmp2);
                        else
                            setCurrentFile((p < 0.65) ? tmp2 : tmp);
                        return true;
                    }
                }
                setCurrentFile(tmp);
                return true;
            }
        }

        setCurrentFile(nullptr);
        return false;
    }
    else
    {
        QList<TreeItem*> all = m_current_tree_model->getAllTrees();
        if(all.isEmpty())
        {
            setCurrentFile(nullptr);
            return false;
        }
        QList<TreeItem*>::iterator iter;
        QFileInfo info;

        for(iter = all.begin(); iter != all.end(); iter++)
        {
            if((*iter) == m_current_play_file)
                break;
        }

        if(iter == all.end())
            iter = all.begin();

        bool exists;
        do
        {
            if(iter == all.begin())
                iter = all.end();

            iter--;

            if((*iter) == m_current_play_file)
            {
                setCurrentFile(nullptr);
                return false;
            }
            info.setFile((*iter)->data(0).toString());
            exists = info.exists();
            if(!exists) (*iter)->setData(4, false);
        }while(!exists || info.isDir());

        setCurrentFile(*iter);
        return true;
    }
    return false;
}

void MainWindow::setCurrentFile(TreeItem *item, bool play_queue) //item musi odwoływać się do istniejącego pliku lub nullptr
{

    if(m_successfully_initialized && m_current_stream_handle != 0)
    {
        onStopButton();
        BASS_StreamFree(m_current_stream_handle);
    }
    m_current_play_file = item;
    if(m_current_play_file == nullptr)
    {
        m_l_title->setText("<html><head/><body><p align=\"center\"><span style=\" font-size:11pt;\">Nieznany Plik</span></p></body></html>");
        return;
    }
    m_current_file_tree->setFocus();
    m_lv_play_queue->setFocus();
    if(play_queue)
        m_play_queue_model->setCurrentItem(item);
    else
        m_current_tree_model->setCurrentItem(item);
    m_hs_track_position->setFocus();
    m_hs_track_position->setSliderPosition(0);

    if(m_successfully_initialized)
    {
        if(m_current_file_name != nullptr)
        {
            delete m_current_file_name;
            m_current_file_name = nullptr;
        }

        #ifdef Q_OS_LINUX
        m_current_stream_handle = BASS_StreamCreateFile(FALSE, m_current_play_file->data(0).toString().toStdString().c_str(), 0, 0, 0); //PROBLEM z WAV
        #endif
        #ifdef Q_OS_WIN32
        QString file_name = m_current_play_file->data(0).toString();
        int length = file_name.size();
        m_current_file_name = new wchar_t[length + 1];
        file_name.toWCharArray(m_current_file_name);
        m_current_file_name[length] = '\0';

        m_current_stream_handle = BASS_StreamCreateFile(FALSE, m_current_file_name, 0, 0, 0); //PROBLEM z WAV
        #endif

        //BASS_StreamCreateFile(stream from memory, path, offset, length (0 = all), flags)
        if(!m_current_stream_handle)
        {
            m_duration = 0.0;
            m_l_title->setText("<html><head/><body><p align=\"center\"><span style=\" font-size:11pt;\">Nieznany Plik</span></p></body></html>");
            return;
        }
        else
        {
            QWORD length = BASS_ChannelGetLength(m_current_stream_handle, BASS_POS_BYTE);
            m_duration = BASS_ChannelBytes2Seconds(m_current_stream_handle, length);
            m_hs_track_position->setMaximum(m_duration * 1000); // w milisekundach
            m_current_play_file->setData(2, m_duration * 1000);
            m_current_play_file->setData(3, m_current_play_file->data(3).toInt() + 1);

            onMusicPositionChange();
        }
    }

    QFileInfo info(m_current_play_file->data(0).toString());
    QString title = "<html><head/><body><p align=\"center\"><span style=\" font-size:11pt;\">" +
            m_current_play_file->data(1).toString() + " [" + info.dir().dirName() + "]" +
            "</span></p></body></html>";
    m_l_title->setText(title);

    setVolumeLevel();
}

void MainWindow::setTimeCounterAndDuration(qint64 pos, qint64 duration)
{
    QString txt = QString("%1:%2%3 / %4:%5%6").arg(pos/60000).arg(((pos/1000) % 60 < 10) ? "0" : "").arg((pos/1000) % 60).arg(duration/60000).arg(((duration/1000) % 60 < 10) ? "0" : "").arg((duration/1000) % 60);
    m_l_time_count_duration->setText(txt);
}

void MainWindow::saveAppConfig()
{
    #ifdef Q_OS_LINUX
    QString ini_path(getenv("HOME"));
    ini_path += "/.config";
    #endif
    #ifdef Q_OS_WIN32
    QString ini_path(QCoreApplication::applicationDirPath());
    #endif
    ini_path += "/ObywatelGCC/OGCCPlayer/";

    QDir dir(ini_path);
    dir.mkdir(ini_path);//tworzymy jeśli nie było

    QDomDocument document;
    QDomElement root = document.createElement("Config");
    document.appendChild(root);

    QDomElement node = document.createElement("MusicBasePath");
    node.setAttribute("absolut_path", m_music_base_path);
    root.appendChild(node);

    QFile xml_file(ini_path + "config.xml");
    if(xml_file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&xml_file);
        stream << document.toString(4);
        xml_file.close();
    }
}

void MainWindow::loadAppConfig()
{
    #ifdef Q_OS_LINUX
    QString ini_path(getenv("HOME"));
    ini_path += "/.config";
    #endif
    #ifdef Q_OS_WIN32
    QString ini_path(QCoreApplication::applicationDirPath());
    #endif
    ini_path += "/ObywatelGCC/OGCCPlayer/";

    QDir dir(ini_path);
    bool created = dir.mkpath(ini_path);//tworzymy jeśli nie było

    QDomDocument document;

    QFile file(ini_path + "config.xml");

    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if(document.setContent(&file))
        {
            file.close();

            QDomNode root = document.firstChild();
            QDomElement element = root.firstChildElement("MusicBasePath");
            m_music_base_path = element.attribute("absolut_path", m_music_base_path);
        }
        else
        {
            file.close();
        }
    }
}

void MainWindow::savePlayLists()
{
    #ifdef Q_OS_LINUX
    QString ini_path(getenv("HOME"));
    ini_path += "/.config";
    #endif
    #ifdef Q_OS_WIN32
    QString ini_path(QCoreApplication::applicationDirPath());
    #endif
    ini_path += "/ObywatelGCC/OGCCPlayer/playlists/";

    class Save
    {
    public:
        static void rek(const TreeItem* parent, QDomElement& dom_parent, QDomDocument& doc)
        {
            if(parent == nullptr) return;

            QList<TreeItem*> tree_list = parent->getChildren();
            QList<TreeItem*>::iterator tree_iter;

            for(tree_iter = tree_list.begin(); tree_iter != tree_list.end(); tree_iter++)
            {
                if((*tree_iter)->isDir())
                {
                    QDomElement node = doc.createElement("dir");
                    node.setAttribute("absolut_path", (*tree_iter)->data(0).toString());
                    node.setAttribute("file_name", (*tree_iter)->data(1).toString());
                    dom_parent.appendChild(node);
                    rek(*tree_iter, node, doc);
                }
                else
                {
                    QDomElement node = doc.createElement("file");
                    node.setAttribute("absolut_path", (*tree_iter)->data(0).toString());
                    node.setAttribute("file_name", (*tree_iter)->data(1).toString());
                    node.setAttribute("playback_count", (*tree_iter)->data(3).toString());
                    dom_parent.appendChild(node);
                }
            }
        }
    };

    QDir dir(ini_path);
    dir.mkdir(ini_path);//tworzymy jeśli nie było
    foreach (QFileInfo file, dir.entryInfoList(QDir::Files | QDir::NoDot | QDir::NoDotDot, QDir::Name))
    {
        QFile::remove(file.absoluteFilePath());
    }

    int tab_count = m_tw_play_lists->count();
    for(int i = 0; i < tab_count; i++)
    {
//        try
//        {
        m_current_file_tree = dynamic_cast<MyTreeView*>(m_tw_play_lists->widget(i));
        m_current_tree_model = dynamic_cast<MyTreeModel*>(m_current_file_tree->model());

        QDomDocument document;
        QDomElement root = document.createElement("Playlist");
        root.setAttribute("name", m_tw_play_lists->tabText(i));
        document.appendChild(root);

        Save::rek(m_current_tree_model->getRootItem(), root, document);

        QFile xml_file(ini_path + QString("playlist_%1.xml").arg(i));
        if(xml_file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream stream(&xml_file);
            stream << document.toString(4);
            xml_file.close();
        }

//        }
//        catch (const std::bad_cast& e)
//        {
//            m_current_file_tree = nullptr;
//            m_current_tree_model = nullptr;
//        }
    }
}

void MainWindow::loadPlayLists()
{
    #ifdef Q_OS_LINUX
    QString ini_path(getenv("HOME"));
    ini_path += "/.config";
    #endif
    #ifdef Q_OS_WIN32
    QString ini_path(QCoreApplication::applicationDirPath());
    #endif
    ini_path += "/ObywatelGCC/OGCCPlayer/playlists/";

    QDir dir(ini_path);
    dir.mkdir(ini_path);//tworzymy jeśli nie było
    QFileInfoList playlists = dir.entryInfoList(QDir::Files | QDir::NoDot | QDir::NoDotDot, QDir::Name);

    class Load
    {
    public:
        static void rek(TreeItem* parent, QDomNode& dom_parent, bool success)
        {
            if(parent == nullptr) return;
            QDomNodeList node_list = dom_parent.childNodes();
            for(int i = 0; i < node_list.size(); i++)
            {
                QDomNode node = node_list.at(i);
                QDomElement e = node.toElement();
                if(e.tagName() == "dir")
                {
                    QVector<QVariant> data;
                    data << e.attribute("absolut_path") << e.attribute("file_name") << QDateTime(QDate(1, 1, 1), QTime(0, 0, 0, 0)) << QVariant("") << QVariant(true);
                    TreeItem* newItem = new TreeItem(data, true, parent);
                    parent->insertChild(-1, newItem);

                    rek(newItem, node, success);

                    parent->setData(2, parent->data(2).toULongLong() + newItem->data(2).toULongLong());
                }
                else if(e.tagName() == "file")
                {
                    QFileInfo info(e.attribute("absolut_path"));
                    bool exists = info.exists();
                    QVector<QVariant> data;
                    data << e.attribute("absolut_path") << e.attribute("file_name") << QDateTime(QDate(1, 1, 1), QTime(0, 0, 0, 0)) << QVariant(e.attribute("playback_count").toInt()) << QVariant(exists);
                    TreeItem* newItem = new TreeItem(data, false, parent);
                    parent->insertChild(-1, newItem);

                    if(exists && success)
                    {
                        #ifdef Q_OS_LINUX
                        HSTREAM str = BASS_StreamCreateFile(FALSE, newItem->data(0).toString().toStdString().c_str(), 0, 0, 0);
                        #endif
                        #ifdef Q_OS_WIN32
                        QString file_name = m_current_play_file->data(0).toString();
                        int length = file_name.size();
                        wchar_t* tmp_current_file_name = new wchar_t[length + 1];
                        file_name.toWCharArray(tmp_current_file_name);
                        m_current_file_name[length] = '\0';

                        HSTREAM str = BASS_StreamCreateFile(FALSE, tmp_current_file_name, 0, 0, 0); //PROBLEM z WAV
                        #endif
                        if(str)
                        {
                            QWORD length = BASS_ChannelGetLength(str, BASS_POS_BYTE);
                            double duration = BASS_ChannelBytes2Seconds(str, length);
                            newItem->setData(2, duration * 1000);
                            parent->setData(2, parent->data(2).toULongLong() + duration * 1000);
                            BASS_StreamFree(str);
                        }
                        #ifdef Q_OS_WIN32
                        delete tmp_current_file_name;
                        #endif
                    }
                }
            }
        }
    };

    QFileInfoList::iterator iter;
    QDomDocument document;

    int i = 0;
    for(iter = playlists.begin(); iter != playlists.end(); iter++)
    {
        QFile file((*iter).absoluteFilePath());
        if(file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            if(document.setContent(&file))
            {
                file.close();

                MyTreeModel* tm = new MyTreeModel(tree_headers);
                MyTreeView* tv = new MyTreeView;

                tv->setContextMenuPolicy(Qt::CustomContextMenu);
                connect(tv, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(fileActive(QModelIndex)));
                connect(tv, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onTreeViewContexttMenu(QPoint)));
                tm->setColumnCount(m_tree_column_count);
                tm->deleteEnable(true);
                tv->setDragEnabled(true);
                tv->setAcceptDrops(true);
                tv->setIndentation(5);
                tv->setAlternatingRowColors(true);

                QDomNode root = document.firstChild();

                Load::rek(tm->getRootItem(), root, m_successfully_initialized);

                tm->getRootItem()->setData(2, tree_headers.at(2));
                tv->setModel(tm);
                tv->setColumnWidth(0, 400);
                m_tw_play_lists->insertTab(i++, tv, root.toElement().attribute("name"));
            }
            else
            {
                file.close();
            }
        }
    }

    if(i == 0)
    {
        MyTreeModel* tm = new MyTreeModel(tree_headers);
        MyTreeView* tv = new MyTreeView;

        tv->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tv, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(fileActive(QModelIndex)));
        connect(tv, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onTreeViewContexttMenu(QPoint)));
        tm->setColumnCount(m_tree_column_count);
        tm->deleteEnable(true);
        tv->setAcceptDrops(true);
        tv->setIndentation(5);
        tv->setModel(tm);
        tv->setColumnWidth(0, 400);
        tv->setAlternatingRowColors(true);

        m_tw_play_lists->insertTab(0, tv, "PlayList");

        m_current_file_tree = tv;
        m_current_tree_model = tm;
    }

//    try
//    {
    m_current_file_tree = dynamic_cast<MyTreeView*>(m_tw_play_lists->widget(0));
    m_current_tree_model = dynamic_cast<MyTreeModel*>(m_current_file_tree->model());

//    }
//    catch (const std::bad_cast& e)
//    {
//        m_current_file_tree = nullptr;
//        m_current_tree_model = nullptr;
//    }
}

void MainWindow::setShortcuts()
{
    m_pb_play->setShortcut(QKeySequence(Qt::Key_Space));
    m_pb_pause->setShortcut(QKeySequence(Qt::Key_P));
    m_pb_stop->setShortcut(QKeySequence(Qt::Key_S));
    m_pb_next->setShortcut(QKeySequence(Qt::Key_K));
    m_pb_previous->setShortcut(QKeySequence(Qt::Key_J));
    m_pb_random_order->setShortcut(QKeySequence(Qt::Key_R));
    m_pb_simple_order->setShortcut(QKeySequence(Qt::Key_E));

    QAction* append_to_queue = new QAction(this);
    append_to_queue->setShortcut(QKeySequence(Qt::Key_Q));
    connect(append_to_queue, SIGNAL(triggered()), this, SLOT(onAppendSelectedToPlayQueue()));
    this->addAction(append_to_queue);

    QAction* delete_action = new QAction(this);
    delete_action->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(delete_action, SIGNAL(triggered()), this, SLOT(onDeleteItem()));
    this->addAction(delete_action);

    QAction* search_action = new QAction(this);
    search_action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
    connect(search_action, SIGNAL(triggered()), this, SLOT(onSearchDialogOpen()));
    this->addAction(search_action);

    QAction* open_select_action = new QAction(this);
    open_select_action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
    connect(open_select_action, SIGNAL(triggered()), this, SLOT(onOpenSelectFile()));
    this->addAction(open_select_action);

    QAction* open_current_action = new QAction(this);
    open_current_action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
    connect(open_current_action, SIGNAL(triggered()), this, SLOT(onOpenCurrentFile()));
    this->addAction(open_current_action);
}

void MainWindow::setVolumeLevel()
{
    if(m_successfully_initialized != 0 && m_current_stream_handle != 0)
        BASS_ChannelSetAttribute(m_current_stream_handle, BASS_ATTRIB_VOL, m_is_mute ? 0.0 : m_volume_level);
}

void MainWindow::setMusicBase()
{
    QString str = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(!str.isEmpty())
    {
        m_music_base_path = str;
        m_le_music_base_path->setText(m_music_base_path);
        setDirModel();
    }
}

void MainWindow::pathEditingFinish()
{
    QFileInfo info(m_le_music_base_path->text());

    if(info.exists())
    {
        m_music_base_path = m_le_music_base_path->text();
        m_le_music_base_path->setText(m_music_base_path);
        setDirModel();
    }
}

void MainWindow::fileActive(const QModelIndex &index)
{
    TreeItem* item = reinterpret_cast<TreeItem*>(index.internalPointer());
    if(item)
        if(!item->isDir())
        {
            setCurrentFile(item);
            if(m_current_stream_handle != 0)
                onPlayButton();
            else
            {
                m_current_play_file->setData(4, false);
                onNextButton();
            }
        }
}

void MainWindow::onPlayButton()
{
    m_continue_playing = false;
    if(!m_successfully_initialized) return;
    if(m_current_play_file == nullptr)
        if(!getNextFile())return;

    if(m_current_stream_handle != 0)
    {
        BASS_ChannelPlay(m_current_stream_handle,FALSE); // FALSE = Don't Restart playback from the beginning
        m_is_playing = true;
        m_track_finished = false;
        m_update_track_position->start();
        m_current_play_file->setData(4, true);
    }
    else
    {
        m_current_play_file->setData(4, false);
    }
}

void MainWindow::onPauseButton()
{
    if(!m_successfully_initialized) return;
    if(m_current_stream_handle != 0)
    {
        BASS_ChannelPause(m_current_stream_handle);
        m_is_playing = false;
    }
}

void MainWindow::onStopButton()
{
    if(!m_successfully_initialized) return;
    if(m_current_stream_handle != 0)
    {
        m_is_playing = false;
        BASS_ChannelPlay(m_current_stream_handle,TRUE); // TRUE = odtwarzaj od początku (skok na początek)
        BASS_ChannelStop(m_current_stream_handle);
        m_hs_track_position->setValue(0);
        onSliderPositionChange(0);
        m_moved_slider = false;
        m_update_track_position->stop();
    }
}

void MainWindow::onNextButton()
{
    if(getNextFile())
        onPlayButton();
}

void MainWindow::onPreviousButton()
{
    if(getPreviousFile())
        onPlayButton();
}

void MainWindow::onVolumeButton()
{
    if(!m_pb_volume->isChecked())
    {
        m_is_mute = false;
        m_pb_volume->setIcon(QIcon(":/res/buttons/sound_on.svg"));
    }
    else
    {
        m_is_mute = true;
        m_pb_volume->setIcon(QIcon(":/res/buttons/sound_off.svg"));
    }

    setVolumeLevel();
}

void MainWindow::onMusicPositionChange()
{
    if(m_moved_slider) return;
    if(!m_successfully_initialized || m_current_stream_handle == 0) return;

    if(m_continue_playing)
        onPlayButton();

    double pos = BASS_ChannelBytes2Seconds(m_current_stream_handle, BASS_ChannelGetPosition(m_current_stream_handle, BASS_POS_BYTE));
    m_duration = BASS_ChannelBytes2Seconds(m_current_stream_handle, BASS_ChannelGetLength(m_current_stream_handle, BASS_POS_BYTE));

    int newpos = pos / m_duration * m_hs_track_position->maximum(); //milisekundy
    m_hs_track_position->setSliderPosition(newpos); //milisekundy
    setTimeCounterAndDuration(newpos, m_duration * 1000);

    if(m_order != Order::None && pos + 0.1 >= m_duration && m_duration > 0 && m_is_playing) // m_duration jest na początku zawyżone
    {
        QList<TreeItem*> trees = m_play_queue_model->getTrees();
        if(!trees.isEmpty())
            if(m_current_play_file == trees.at(0))
                m_play_queue_model->removeRows(0, 1);

        onNextButton();
    }
}

void MainWindow::onSliderPositionChange(int pos) //milisekundy
{
    m_moved_slider = true;
    double newpos = m_duration * pos / m_hs_track_position->maximum(); //sekundy

    if(m_successfully_initialized && m_current_stream_handle != 0)
    {
        setTimeCounterAndDuration(newpos * 1000, m_duration * 1000);
        BASS_ChannelSetPosition(m_current_stream_handle, BASS_ChannelSeconds2Bytes(m_current_stream_handle, newpos), BASS_POS_BYTE);
    }
}

void MainWindow::onVolumePositionChange(int pos)
{
    m_volume_level = static_cast<float>(pos) / m_hs_volume_position->maximum();
    setVolumeLevel();
}

void MainWindow::onSliderPress()
{
    m_moved_slider = true;
    if(m_is_playing)
    {
        onPauseButton();
        m_continue_playing = true;
    }
    else
        onPauseButton();
}

void MainWindow::onSliderRelease()
{
    onSliderPositionChange(m_hs_track_position->value());
    m_moved_slider = false;
    if(m_continue_playing)
    {
        onPlayButton();
    }
}

void MainWindow::onCurrentTabChanged(int index)
{
    if(index == -1) return;
//    try
//    {
        m_current_file_tree = dynamic_cast<MyTreeView*>(m_tw_play_lists->widget(index));
        m_current_tree_model = dynamic_cast<MyTreeModel*>(m_current_file_tree->model());
        m_current_play_file = m_current_tree_model->getCurrentItem();
//    }
//    catch (const std::bad_cast& e)
//    {
//        m_current_file_tree = nullptr;
//        m_current_tree_model = nullptr;
//    }
}

void MainWindow::onAddPlayListClicked()
{
    MyTreeModel* tm = new MyTreeModel(tree_headers);
    MyTreeView* tv = new MyTreeView;

    tv->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tv, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(fileActive(QModelIndex)));
    connect(tv, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onTreeViewContexttMenu(QPoint)));
    tm->setColumnCount(m_tree_column_count);
    tm->deleteEnable(true);
    tv->setDragEnabled(true);
    tv->setAcceptDrops(true);
    tv->setIndentation(5);
    tv->setAlternatingRowColors(true);

    tv->setModel(tm);
    tv->setColumnWidth(0, 400);

    m_tw_play_lists->insertTab(m_tw_play_lists->count(), tv, "Playlista");
    m_tw_play_lists->setCurrentIndex(m_tw_play_lists->count() - 1);
    m_current_file_tree = tv;
    m_current_tree_model = tm;
}

void MainWindow::onPlaybackOrder()
{
    if(m_pb_simple_order->isChecked() && m_order != Order::Simple)
    {
        m_pb_random_order->setChecked(false);
        m_order = Order::Simple;
    }
    else if(m_pb_random_order->isChecked() && m_order != Order::Random)
    {
        m_pb_simple_order->setChecked(false);
        m_order = Order::Random;
    }
    else
    {
        m_order = Order::None;
    }
}

void MainWindow::onAppendSelectedToPlayQueue()
{
    m_current_file_tree->setFocus();
    m_lv_play_queue->setFocus();
    m_hs_track_position->setFocus();

    QMimeData *mime = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::ReadWrite);
    QModelIndex i = m_current_file_tree->firstSelectedIndex();
    TreeItem* item;
    quintptr address;
    if(i.isValid())
    {
        item = reinterpret_cast<TreeItem*>(i.internalPointer());
        address = reinterpret_cast<quintptr>(item);
        stream << address;
        address = reinterpret_cast<quintptr>(m_current_tree_model);
        stream << address;

        QVector<int> path = item->getPath();
        QVector<int>::iterator iter;
        for(iter = path.begin(); iter != path.end(); iter++)
            stream << *iter;
    }

    mime->setData("application/treeitem.ptr", encodedData);

    m_play_queue_model->dropMimeData(mime, Qt::MoveAction, -1, 0, QModelIndex());
}

void MainWindow::onDeleteItem()
{
    QWidget* w = this->focusWidget();
    if(w == m_current_file_tree)
    {
        QModelIndex index = m_current_file_tree->firstSelectedIndex();
        if(m_current_play_file == reinterpret_cast<TreeItem*>(index.internalPointer()))
        {
            onStopButton();
            setCurrentFile(nullptr);
            m_current_tree_model->setCurrentItem(nullptr);
        }
        m_current_tree_model->removeRows(index.row(), 1, index.parent());
    }
    else if(w == m_lv_play_queue)
    {
        QModelIndex index = m_lv_play_queue->firstSelectedIndex();
        if(m_current_play_file == reinterpret_cast<TreeItem*>(index.internalPointer()))
        {
            onStopButton();
            setCurrentFile(nullptr);
            m_play_queue_model->setCurrentItem(nullptr);
        }
        m_play_queue_model->removeRows(index.row(), 1, index.parent());
    }
}

void MainWindow::onSearchDialogOpen()
{
    m_search_dialog->setTreeModel(m_current_tree_model);
    m_search_dialog->show();
}

void MainWindow::onOpenSelectFile()
{
    QModelIndex firest_selected = m_current_file_tree->firstSelectedIndex();
    if(!firest_selected.isValid()) return;
    TreeItem* item = reinterpret_cast<TreeItem*>(firest_selected.internalPointer());
    QFileInfo info(item->data(0).toString());
    QString c = "dolphin \"%1\" &";
    if(info.exists())
    {
        if(item->isDir())
        {
            c = c.arg(item->data(0).toString());
            system(c.toStdString().c_str());

        }
        else
        {
            c = c.arg(info.path());
            system(c.toStdString().c_str());
        }
    }
    else if(!m_music_base_path.isEmpty())
    {
        c = c.arg(m_music_base_path);
        system(c.toStdString().c_str());
    }
}

void MainWindow::onOpenCurrentFile()
{
    QFileInfo info(m_current_play_file->data(0).toString());
    QString c = "dolphin \"%1\" &";
    if(info.exists())
    {
        if(m_current_play_file->isDir())
        {
            c = c.arg(m_current_play_file->data(0).toString());
            system(c.toStdString().c_str());

        }
        else
        {
            c = c.arg(info.path());
            system(c.toStdString().c_str());
        }
    }
    else if(!m_music_base_path.isEmpty())
    {
        c = c.arg(m_music_base_path);
        system(c.toStdString().c_str());
    }
}

void MainWindow::onScrollTrackForward()
{

}

void MainWindow::onScrollTrackBackward()
{
    m_moved_slider = true;
    if(m_is_playing)
    {
        onPauseButton();
        m_is_playing = true;
    }
    else
        onPauseButton();

    onSliderPositionChange(m_hs_track_position->value() - 5 * 1000);

    m_moved_slider = false;
    if(m_is_playing)
        onPlayButton();
}

void MainWindow::onSearchDialogActiveFile(TreeItem *item, bool add_to_queue)
{
    if(add_to_queue)
    {
        m_current_file_tree->setFocus();
        m_lv_play_queue->setFocus();
        m_hs_track_position->setFocus();

        QMimeData *mime = new QMimeData();
        QByteArray encodedData;
        QDataStream stream(&encodedData, QIODevice::ReadWrite);
        quintptr address;
        if(item != nullptr)
        {
            address = reinterpret_cast<quintptr>(item);
            stream << address;
            address = reinterpret_cast<quintptr>(m_current_tree_model);
            stream << address;

            QVector<int> path = item->getPath();
            QVector<int>::iterator iter;
            for(iter = path.begin(); iter != path.end(); iter++)
                stream << *iter;

            mime->setData("application/treeitem.ptr", encodedData);
            m_play_queue_model->dropMimeData(mime, Qt::MoveAction, -1, 0, QModelIndex());
        }
    }
    else
    {
        setCurrentFile(item);
        onPlayButton();
    }
}

void MainWindow::onTabBarContextMenu(const QPoint &pos)
{
    if(pos.isNull()) return;

    int current_index = m_tb_play_lists->tabAt(pos);
    if(current_index == -1) return;

    QMenu menu(this);
    QAction* change_name = menu.addAction("Zmień nazwę");
    QAction* delete_tab = menu.addAction("Usuń");
    QAction* clear_tab = menu.addAction("Wyczyść");
    QAction* delete_invalid_and_empty = menu.addAction("Usuń złe pliki i puste foldery");

    QAction* choose = menu.exec(m_tb_play_lists->mapToGlobal(pos));
    if(choose == change_name)
    {
        NewNameDialog name_dialog(m_tw_play_lists->tabText(current_index));
        name_dialog.setModal(true);
        name_dialog.exec();

        if(name_dialog.result() == QDialog::Accepted)
        {
            m_tw_play_lists->setTabText(current_index, name_dialog.getName());
        }
    }
    else if(choose == delete_tab)
    {
//        try
//        {
        MyTreeView* tmp_tree = dynamic_cast<MyTreeView*>(m_tw_play_lists->widget(current_index));
        MyTreeModel* tmp_model = dynamic_cast<MyTreeModel*>(m_current_file_tree->model());
        if(tmp_tree == m_current_file_tree)
        {
            m_current_file_tree = nullptr;
            m_current_tree_model = nullptr;
        }
        m_tw_play_lists->removeTab(current_index);

//        }
//        catch (const std::bad_cast& e)
//        {
//            m_current_file_tree = nullptr;
//            m_current_tree_model = nullptr;
//        }
    }
    else if(choose == clear_tab)
    {
//        try
//        {
        MyTreeView* tree = dynamic_cast<MyTreeView*>(m_tw_play_lists->widget(current_index));
        MyTreeModel* model = dynamic_cast<MyTreeModel*>(tree->model());
        if(model == m_current_tree_model)
            setCurrentFile(nullptr);

        model->deleteChildren();
//        }
//        catch (const std::bad_cast& e)
//        {
//            m_current_file_tree = nullptr;
//            m_current_tree_model = nullptr;
//        }
    }
    else if(choose == delete_invalid_and_empty)
    {
//        try
//        {
        MyTreeView* tree = dynamic_cast<MyTreeView*>(m_tw_play_lists->widget(current_index));
        MyTreeModel* model = dynamic_cast<MyTreeModel*>(tree->model());
        model->removeAllRowsWithValue(4, false);

//        }
//        catch (const std::bad_cast& e)
//        {
//            m_current_file_tree = nullptr;
//            m_current_tree_model = nullptr;
//        }
    }
}

void MainWindow::onTreeViewContexttMenu(const QPoint &pos)
{
    if(pos.isNull()) return;
    if(m_current_file_tree == nullptr) return;

    QModelIndex index = m_current_file_tree->indexAt(pos);
    if(!index.isValid()) return;

    QMenu menu(this);
    QAction* delete_index = menu.addAction("Usuń");
    QAction* open_in_folder = menu.addAction("Otwórz w folderze");
    QAction* add_to_queue = menu.addAction("Dodaj do kolejki odtwarzania");

    QAction* choose = menu.exec(m_current_file_tree->mapToGlobal(pos));
    if(choose == delete_index)
    {
        m_current_tree_model->removeRows(index.row(), 1,index.parent());
    }
    else if(choose == open_in_folder)
    {
        TreeItem* item = reinterpret_cast<TreeItem*>(index.internalPointer());
        QFileInfo info(item->data(0).toString());
        QString c = "dolphin \"%1\" &";
        if(info.exists())
        {
            if(item->isDir())
            {
                c = c.arg(item->data(0).toString());
                system(c.toStdString().c_str());

            }
            else
            {
                c = c.arg(info.path());
                system(c.toStdString().c_str());
            }
        }
        else if(!m_music_base_path.isEmpty())
        {
            c = c.arg(m_music_base_path);
            system(c.toStdString().c_str());
        }
    }
    else if(choose == add_to_queue)
    {
        onAppendSelectedToPlayQueue();
    }
}

void MainWindow::onPlayQueueContexttMenu(const QPoint &pos)
{

}

