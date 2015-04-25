#include "mytreemodel.h"
#include <QIcon>
#include <QMimeData>
#include <QItemSelectionModel>
#include <QUrl>
#include <QDebug>
#include <functional>

MyTreeModel::MyTreeModel(const QStringList &headers, QStringList data, int column_count, QObject *parent)
    : MyDirModel(headers, data, column_count, parent),
      m_curent_item(nullptr)
{
    MyDirModel::deleteEnable(true);
}

MyTreeModel::~MyTreeModel()
{
}

bool MyTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{

    TreeItem* item,* tmp;
    for(int i = 0; i < count && m_curent_item != nullptr; i++)
    {
        item = getItem(index(row + i, 0, parent));
        tmp = m_curent_item;
        while(tmp != nullptr)
        {
            if(tmp == item)
            {
                m_curent_item = nullptr;
                tmp = nullptr;
            }
            else
                tmp = tmp->parent();
        }
    }
    MyDirModel::removeRows(row, count, parent);
}

QVariant MyTreeModel::data(const QModelIndex &index, int role) const
{
    if(index.column() == 1 && role == Qt::DisplayRole)
    {
        TreeItem* item = getItem(index);
        unsigned long long allmilliseconds = item->data(2).toLongLong();
        //unsigned milliseconds = allmilliseconds % 1000;
        unsigned seconds = (allmilliseconds / 1000) % 60;
        unsigned minutes = (allmilliseconds / 60000) % 60;
        unsigned hours = (allmilliseconds / 3600000) % 24;
        unsigned days = allmilliseconds / 3600000 / 24;

        auto flambda = [](unsigned liczba, unsigned min_l_znakow) -> QString
        {
            QString out;
            for(int i = min_l_znakow - 1; i > 0; i--)
                if(liczba < pow(10, i))
                    out += '0';
            out += QString::number(liczba);
            return out;
        };

        if(allmilliseconds < 3600000)
            return QString("%1:%2").arg(flambda(minutes, 2)).arg(flambda(seconds, 2));
        else if(allmilliseconds < (3600000 * 24))
            return QString("%1:%2:%3").arg(flambda(hours, 2)).arg(flambda(minutes, 2)).arg(flambda(seconds, 2));
        else
            return QString("%1d %2:%3:%4").arg(flambda(days, 1)).arg(flambda(hours, 2)).arg(flambda(minutes, 2)).arg(flambda(seconds, 2));

        return QString("%1:%2").arg(flambda(minutes, 2)).arg(flambda(seconds, 2));
    }

    if(role == Qt::TextColorRole)
    {
        TreeItem* item = getItem(index);
        if(item->columnCount() >= 5)
            if(item->data(4).toBool() == false)
                return QColor(Qt::gray);
    }
    if(m_curent_item != nullptr && role == Qt::BackgroundRole)
    {
        TreeItem* item = getItem(index);
        if(item == m_curent_item)
            return QColor(70, 190, 223, 180);

        TreeItem* parent = m_curent_item->parent();
        while(parent != nullptr)
        {
            if(parent == item)
                return QColor(70, 190, 223, 80);
            parent = parent->parent();
        }
    }
    return MyDirModel::data(index, role);
}


bool MyTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return false;
}

Qt::ItemFlags MyTreeModel::flags(const QModelIndex &index) const
{
    if (index.isValid())
        return Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | MyDirModel::flags(index);

    return Qt::ItemIsDropEnabled;
}

bool MyTreeModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    return true;
}

bool MyTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if(data->hasFormat("application/treeitem.str")) //przeciągnięcie z drzewa katalogów
    {
        QByteArray byteArray = data->data("application/treeitem.str");
        QDataStream stream(&byteArray, QIODevice::ReadOnly);
        QString tmp = "";
        while (!stream.atEnd())
            stream >> tmp;

        if(tmp != 0)
        {
            auto MergeFunction = [&](TreeItem* parent_item, const QModelIndex& index, bool& merged) -> void
            {
                bool found = false;
                QList<TreeItem*> children = parent_item->getChildren();
                QList<TreeItem*>::iterator iter;
                for(iter = children.begin(); iter != children.end(); iter++)
                {
                    if((*iter)->isDir() && (*iter)->data(0).toString() == tmp)
                    {
                        TreeItem* new_item = createAudioMediaTree(-1, tmp, nullptr);
                        QList<TreeItem*> children2 = new_item->getChildren();
                        QList<TreeItem*> children3 = (*iter)->getChildren();
                        for(int i = 0; i < children2.size();i++)
                        {
                            found = false;
                            for(int j = 0; j < children3.size(); j++)
                            {
                                if(children2.at(i)->data(0).toString() == children3.at(j)->data(0).toString())
                                {
                                    children2.removeAt(i);
                                    new_item->removeChildren(i, 1);
                                    i--;
                                    found = true;
                                    break;
                                }
                            }
                            if(!found)
                            {
                                beginInsertRows(index, (*iter)->childCount(), (*iter)->childCount());
                                moveItem(new_item, i, *iter, (*iter)->childCount());//przenosimy na koniec
                                endInsertRows();
                            }

                        }
                        delete new_item;
                        merged = true;
                        break;
                    }
                }
            };

            if(parent.isValid())
            {
                TreeItem* item = reinterpret_cast<TreeItem*>(parent.internalPointer());
                if(item->isDir())
                {
                    bool merged = false;
                    MergeFunction(item, parent, merged);
                    if(!merged)
                        insertNewTree(-1, tmp, parent, true); //dodajemy na końcu
                }
                else
                    insertNewTree(parent.row(), tmp, parent.parent(), true); //wstawiamy w miejsce

            }
            else
            {
                bool merged = false;
                MergeFunction(rootItem, QModelIndex(), merged);
                if(!merged)
                    insertNewTree(row, tmp, QModelIndex(), true);
            }
        }
        return true;
    }
    else if(data->hasFormat("application/treeitem.ptr")) //przeciągnięcie z play listy
    {
        QByteArray byteArray = data->data("application/treeitem.ptr");
        QDataStream stream(&byteArray, QIODevice::ReadOnly);
        quintptr tmp = 0;
        TreeItem* item;
        int node_number;
        QModelIndex item_index = QModelIndex();
        stream >> tmp;

        while (!stream.atEnd())
        {
            stream >> node_number;
            item_index = index(node_number, 0, item_index);
        }

        if(tmp != 0)
        {
            item = getItem(parent);
            qDebug() << "DROP" << row << parent.row() << item->isDir();
            if(item->isDir()) //przesuwamy na wskazaną pozycję w folderze
                moveTree(item_index.parent(), node_number, parent, (row == -1) ? item->childCount() : row);
            else
                moveTree(item_index.parent(), node_number, parent.parent(), parent.row());
        }
        return true;
    }

    return false;
}

QMimeData *MyTreeModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mime = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::ReadWrite);
    QModelIndex i = indexes.at(0);
    TreeItem* item;
    quintptr address;
    if(i.isValid())
    {
        item = reinterpret_cast<TreeItem*>(i.internalPointer());
        address = reinterpret_cast<quintptr>(item);
        stream << address;
        address = reinterpret_cast<quintptr>(this);
        stream << address;

        QVector<int> path = item->getPath();
        QVector<int>::iterator iter;
        for(iter = path.begin(); iter != path.end(); iter++)
            stream << *iter;
    }

    mime->setData("application/treeitem.ptr", encodedData);
    return mime;
}

TreeItem *MyTreeModel::getRootItem() const
{
    return this->rootItem;
}

void MyTreeModel::appendItem(TreeItem *item)
{
    TreeItem* tmp = item;
    insertItem(tmp, -1, QModelIndex());
}

void MyTreeModel::insertItem(TreeItem *item, int position, const QModelIndex& parent)
{
    if(item == nullptr) return;

    beginInsertRows(parent, position, position);
    TreeItem* p = getItem(parent);
    p->insertChild(position, item);
    endInsertRows();
}

void MyTreeModel::moveTree(const QModelIndex& sourceParent, int position, const QModelIndex& destinyParent, int destiny)
{
    qDebug() << position << destiny;
    //źródło nie może być celem
    if(sourceParent == destinyParent && position == destiny) return; //w przypadku plików
    if(sourceParent == destinyParent.parent() && position == destinyParent.row()) return; //w przypadku folderów

    TreeItem* parent = getItem(sourceParent);
    if(position < 0 || position >= parent->childCount()) return;

    TreeItem* item = parent->child(position);

    bool delete_enable = this->m_delete_enable;
    this->m_delete_enable = false;
    removeRows(position, 1, sourceParent);

    while(parent->parent() != nullptr)
    {
        parent->setData(2, parent->data(2).toULongLong() - item->data(2).toULongLong());
        parent = parent->parent();
    }

    this->m_delete_enable = delete_enable;
    insertItem(item, destiny, destinyParent);

    parent = getItem(destinyParent);
    while(parent->parent() != nullptr)
    {
        parent->setData(2, parent->data(2).toULongLong() + item->data(2).toULongLong());
        parent = parent->parent();
    }
}

void MyTreeModel::setCurrentItem(TreeItem *item)
{
    m_curent_item = item;
}

TreeItem *MyTreeModel::getCurrentItem() const
{
    return m_curent_item;
}

void MyTreeModel::deleteChildren()
{
    m_curent_item = nullptr;
    MyDirModel::deleteChildren();

}

void MyTreeModel::removeAllRowsWithValue(int column, QVariant value)
{
    if(rootItem->columnCount() <= column) return;
    std::function< void(TreeItem*) > removeAllRows = [&](TreeItem* parent) -> void
    {
        QList<TreeItem*> children = parent->getChildren();
        QList<TreeItem*>::iterator iter;
        TreeItem* tmp;
        for(iter = children.begin();iter != children.end(); iter++)
        {
            if((*iter)->isDir())
                removeAllRows(*iter);
            else if((*iter)->data(column) == value)
            {
                if((*iter) == m_curent_item)
                    m_curent_item == nullptr;
                tmp = parent;
                while(tmp != nullptr)
                {
                    tmp->setData(2, tmp->data(2).toInt() - (*iter)->data(2).toInt());
                    tmp = tmp->parent();
                }
                parent->removeChild(iter);
            }
        }
        if(parent->childCount() == 0)
        {
            if(parent->parent() != nullptr)
                parent->parent()->removeChildren(parent->positionOfThis(), 1);
        }
    };

    beginResetModel();
    removeAllRows(rootItem);
    endResetModel();
}

