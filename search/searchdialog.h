#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include "models/mytreemodel.h"

namespace Ui {
class SearchDialog;
}

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = 0);
    ~SearchDialog();
    void setTreeModel(MyTreeModel* model);
    void clearLists();
signals:
    void activeFile(TreeItem* item, bool add_to_queue);

private slots:
    void on_lineEdit_textChanged(const QString &arg1);
    void on_listWidget_itemActivated(QListWidgetItem *item);
    void onAppendSelectedToPlayQueue();

private:

    Ui::SearchDialog *ui;
    MyTreeModel* m_current_playlist_model;

    QStringList tree_headers;
    QList <TreeItem*> m_found_files;
    QListWidget* lw;
};

#endif // SEARCHDIALOG_H
