#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "models/mydirmodel.h"
#include "models/mytreemodel.h"
#include "models/mylistmodel.h"
#include "search/searchdialog.h"
#include "mywidgets.h"

#include <QMainWindow>
#include <QTreeView>
#include <QToolButton>
#include <QListView>
#include <QFileSystemModel>
#include <QLabel>
#include <QTimer>

#include <bass.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void keyReleaseEvent(QKeyEvent *e);

private:
    enum Order
    {
        None,
        Simple,
        Random
    };

    void setDirModel();
    bool getNextFile();
    bool getPreviousFile();
    void setCurrentFile(TreeItem* item, bool play_queue = false);
    void setTimeCounterAndDuration(qint64 pos, qint64 duration);
    void savePlayLists();
    void loadPlayLists();
    void setShortcuts();
    void setVolumeLevel();

private slots:
    void setMusicBase();
    void pathEditingFinish();
    void fileActive(const QModelIndex& index);
    void onPlayButton();
    void onPauseButton();
    void onStopButton();
    void onNextButton();
    void onPreviousButton();
    void onVolumeButton();
    void onMusicPositionChange();
    void onSliderPositionChange(int pos);
    void onVolumePositionChange(int pos);
    void onSliderPress();
    void onSliderRelease();
    void onCurrentTabChanged(int index);
    void onAddPlayListClicked();
    void onPlaybackOrder();
    void onAppendSelectedToPlayQueue();
    void onDeleteItem();
    void onSearchDialogOpen();
    void onOpenSelectFile();
    void onOpenCurrentFile();
    void onScrollTrackForward();
    void onScrollTrackBackward();
    void onSearchDialogActiveFile(TreeItem* item, bool add_to_queue);
    void onTabBarContextMenu(const QPoint& pos);
    void onTreeViewContexttMenu(const QPoint& pos);
    void onPlayQueueContexttMenu(const QPoint& pos);

private:
    Ui::MainWindow *ui;

    QString m_music_base_path;
    QTreeView* m_tv_music_base;
    MyDirModel* m_dir_model;

    MyTreeModel* m_current_tree_model;
    MyListModel* m_play_queue_model;
    MyListView* m_lv_play_queue;
    MyTreeView* m_current_file_tree;
    TreeItem* m_current_play_file;

    //widgets
    QPushButton* m_pb_play;
    QPushButton* m_pb_pause;
    QPushButton* m_pb_stop;
    QPushButton* m_pb_next;
    QPushButton* m_pb_previous;
    QPushButton* m_pb_volume;
    QPushButton* m_pb_simple_order;
    QPushButton* m_pb_random_order;
    QLineEdit* m_le_music_base_path;
    QToolButton* m_tb_set_music_base_path;
    MySlider* m_hs_track_position;
    MySlider* m_hs_volume_position;
    QLabel* m_l_title;
    QLabel* m_l_time_count_duration;
    QTabWidget* m_tw_play_lists;
    QTabBar* m_tb_play_lists;

    //BASS playback
    bool m_successfully_initialized;
    HSTREAM m_current_stream_handle;
    bool m_is_mute;
    float m_volume_level; // 0.0 - 1.0
    double m_duration; // w sekundach
    wchar_t* m_current_file_name;
    Order m_order;

    QTimer* m_update_track_position;
    QStringList tree_headers;

    bool m_moved_slider; //zapomiega sprzeżeniu zwrotnemu
    bool m_is_playing;
    bool m_track_finished;
    bool m_continue_playing;

    int m_tree_column_count;

    SearchDialog* m_search_dialog;
};

#endif // MAINWINDOW_H
