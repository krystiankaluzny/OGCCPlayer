#include "mylistmodel.h"
#include "treeitem.h"

#include <QMimeData>
#include <QItemSelectionModel>
#include <QUrl>
#include <QList>
#include <QDebug>

MyListModel::MyListModel(const QStringList &headers, QStringList data, int column_count, QObject *parent) :
    MyTreeModel(headers, data, column_count, parent)
{
}

bool MyListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    int destiny = (row != -1) ? row : parent.row();

    if(data->hasFormat("application/treeitem.str")) //przeciągnięcie z drzewa katalogów
    {
        QByteArray byteArray = data->data("application/treeitem.str");
        QDataStream stream(&byteArray, QIODevice::ReadOnly);
        QString tmp = "";
        while (!stream.atEnd())
            stream >> tmp;

        if(tmp != 0)
        {
            beginResetModel();
            TreeItem* item = createAudioMediaTree(destiny, tmp, rootItem);
            if(item->isDir())
            {
                QList<TreeItem*> all_children = item->getAllChildren(true);
                QList<TreeItem*>::iterator iter;

                for(iter = all_children.begin(); iter != all_children.end(); iter++)
                {
                    moveItem((*iter)->parent(), (*iter)->positionOfThis(), rootItem, (destiny == -1) ? -1 : destiny++);
                }
                rootItem->removeChildren(item->positionOfThis(), 1);
            }
            endResetModel();
        }
        return true;
    }
    else if(data->hasFormat("application/treeitem.ptr")) //przeciągnięcie z play listy
    {
        QByteArray byteArray = data->data("application/treeitem.ptr");
        QDataStream stream(&byteArray, QIODevice::ReadOnly);
        quintptr tmp = 0;
        quintptr model = 0;
        int node_number;
        QModelIndex item_index = QModelIndex();
        stream >> tmp;
        stream >> model;

        while (!stream.atEnd())
        {
            stream >> node_number;
            item_index = index(node_number, 0, item_index);
        }

        if(tmp != 0)
        {
            if(reinterpret_cast<MyListModel*>(model) == this)
            {
                TreeItem* item = reinterpret_cast<TreeItem*>(tmp);
                beginResetModel();
                if(!item->isDir())
                {
                    moveItem(item->parent(), item->positionOfThis(), rootItem, destiny);
                }
                endResetModel();
            }
            else
            {
                TreeItem* item = reinterpret_cast<TreeItem*>(tmp);
                beginResetModel();
                if(item->isDir())
                {
                    QList<TreeItem*> all_children = item->getAllChildren(true);
                    QList<TreeItem*>::iterator iter;

                    for(iter = all_children.begin(); iter != all_children.end(); iter++)
                    {
                        (*iter)->setData(3, (*iter)->data(3).toInt() + 1);
                        createAudioMediaTree((destiny == -1) ? -1 : destiny++, (*iter)->data(0).toString(), rootItem);
                    }
                }
                else
                {
                    item->setData(3, item->data(3).toInt() + 1);
                    createAudioMediaTree(destiny, item->data(0).toString(), rootItem);
                }
                endResetModel();
            }
        }
        return true;
    }

    return false;
}
