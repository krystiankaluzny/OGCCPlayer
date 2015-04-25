#include "searchdialog.h"
#include "ui_searchdialog.h"
#include <QFileInfo>
#include <QDir>

SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog),
    m_current_playlist_model(nullptr)
{
    ui->setupUi(this);
    lw = ui->listWidget;

    QAction* append_to_queue = new QAction(this);
    append_to_queue->setShortcut(QKeySequence(Qt::Key_Q));
    connect(append_to_queue, SIGNAL(triggered()), this, SLOT(onAppendSelectedToPlayQueue()));
    this->addAction(append_to_queue);
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::setTreeModel(MyTreeModel *model)
{
    clearLists();
    m_current_playlist_model = model;
}

void SearchDialog::clearLists()
{
    m_found_files.clear();
    lw->clear();
}

void SearchDialog::on_lineEdit_textChanged(const QString &arg1)
{
    if(m_current_playlist_model == nullptr) return;
    if(!arg1.isEmpty())
    {
        clearLists();
        QList<TreeItem*> all = m_current_playlist_model->getAllTrees();
        QList<TreeItem*>::iterator iter;
        for(iter = all.begin(); iter != all.end(); iter++)
            if((*iter)->data(1).toString().toLower().contains(arg1))
                m_found_files.append(*iter);

        for(iter = m_found_files.begin(); iter != m_found_files.end(); iter++)
        {
            QListWidgetItem *item = new QListWidgetItem;
            QFileInfo info((*iter)->data(0).toString());
            item->setText((*iter)->data(1).toString() + " [" + info.dir().dirName() + "]");
            item->setData(Qt::WhatsThisRole, reinterpret_cast<qintptr>(*iter));
            lw->addItem(item);
        }
    }
}

void SearchDialog::on_listWidget_itemActivated(QListWidgetItem *item)
{
    TreeItem* it = reinterpret_cast<TreeItem*>(item->data(Qt::WhatsThisRole).value<qintptr>());
    emit activeFile(it, false);
}

void SearchDialog::onAppendSelectedToPlayQueue()
{
    QList<QListWidgetItem *> sl = lw->selectedItems();
    if(!sl.isEmpty())
    {
        TreeItem* it = reinterpret_cast<TreeItem*>(sl.at(0)->data(Qt::WhatsThisRole).value<qintptr>());
        emit activeFile(it, true);
    }
}
