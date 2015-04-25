#include "mydirmodel.h"
#include <QDir>
#include <QFileInfoList>
#include <QIcon>
#include <QMimeData>
#include <QByteArray>
#include <QDataStream>
#include <QUrl>
#include <QDebug>

MyDirModel::MyDirModel(const QStringList &headers, QStringList data, int column_count, QObject *parent)
    : QAbstractItemModel(parent),
      m_delete_enable(true),
      m_column_count(column_count)
{
    QVector<QVariant> rootData;
    for(QString header : headers)
        rootData << header;

    rootItem = new TreeItem(rootData);
    rootItem->setDir(true);
    for(int i = 0; i < data.size(); i++)
        createDirTree(-1, data.at(i), rootItem);
}

MyDirModel::~MyDirModel()
{
    deleteChildren();

    delete rootItem;
}

int MyDirModel::rowCount(const QModelIndex &parent) const
{
    TreeItem* parentItem = getItem(parent);
    return parentItem->childCount();
}

int MyDirModel::columnCount(const QModelIndex &parent) const
{
    return m_column_count > 0 ? m_column_count : rootItem->columnCount() - 1;
}

QVariant MyDirModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role == Qt::DecorationRole && index.column() == 0)
    {
        bool is_dir = reinterpret_cast<TreeItem*>(index.internalPointer())->isDir();
        QIcon icon;
        if(is_dir)
            icon.addFile(":/res/folder.png");
        else
            icon.addFile(":/res/text-plain.png");
        return icon;
    }

    if(role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem* item = getItem(index);

    return item->data(index.column() + 1);
}

bool MyDirModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role != Qt::EditRole)
        return false;

    TreeItem* item = getItem(index);
    bool result;

    if(index.column() == 2 || index.column() == 3)
        result = item->setDownTree(index.column() + 1, value);
    else
        result = item->setData(index.column() + 1, value);


    if(result)
        emit dataChanged(index, index);

    return result;
}

Qt::ItemFlags MyDirModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;

    int c = index.column();

    if(c == 0)
        return Qt::ItemIsDragEnabled | QAbstractItemModel::flags(index);

    if(c == 2 || c == 3)
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);

    return QAbstractItemModel::flags(index);
}

TreeItem *MyDirModel::getItem(const QModelIndex &index) const
{
    if(index.isValid())
    {
        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
        if(item)
            return item;
    }
    return rootItem;
}

QVariant MyDirModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section + 1);

    return QVariant();
}

bool MyDirModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if(role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if(result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

QModelIndex MyDirModel::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem* parentItem = getItem(parent);

    TreeItem* childItem = parentItem->child(row);

    if(childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex MyDirModel::parent(const QModelIndex &child) const
{
    if(!child.isValid())
        return QModelIndex();

    TreeItem* childItem = getItem(child);
    TreeItem* parentItem = childItem->parent();

    if(parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->positionOfThis(), 0, parentItem);
}

bool MyDirModel::insertRows(int row, int count, const QModelIndex &parent)
{
    TreeItem* parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, row, row + count - 1);
    success = parentItem->insertChildren(row, count, rootItem->columnCount());
    endInsertRows();

    return success;
}

bool MyDirModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(count < 1) return false;

    TreeItem* parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, row, row + count - 1);
    if(m_delete_enable)
        success = parentItem->removeChildren(row, count);
    else
        success = parentItem->clearChildren(row, count);
    endRemoveRows();

    return success;
}

TreeItem* MyDirModel::appendNewTree(QString &path, bool create_audio)
{
    return insertNewTree(rootItem->childCount(), path, QModelIndex(), create_audio);
}

TreeItem *MyDirModel::insertNewTree(int position, QString &path, const QModelIndex& parent, bool create_audio)
{
    TreeItem* tree;

    beginInsertRows(parent, position, position);
    if(create_audio)
        tree = createAudioMediaTree(position, path, getItem(parent));
    else
        tree = createDirTree(position, path, getItem(parent));
    endInsertRows();

    return tree;
}

QList<TreeItem*> MyDirModel::getTrees() const
{
    return rootItem->getChildren();
}

QList<TreeItem *> MyDirModel::getAllTrees(bool without_dirs) const
{
    return rootItem->getAllChildren(without_dirs);
}

void MyDirModel::deleteChildren()
{
    beginResetModel();
    if(m_delete_enable)
        rootItem->removeAllChildren();
    else
        rootItem->clearAllChildren();
    endResetModel();
}

void MyDirModel::setColumnCount(int count)
{
    beginResetModel();
    m_column_count = count;
    endResetModel();
}

void MyDirModel::deleteEnable(bool can)
{
//należy ustawić na fałsz jeżeli drzewa są tworzone na zewnątrz modelu i sami dbamy o ich usunięcie
    m_delete_enable = can;
}

Qt::DropActions MyDirModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList MyDirModel::mimeTypes() const
{
    QStringList types;
    types << "application/treeitem.str" << "application/treeitem.ptr";
    return types;
}

QMimeData* MyDirModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mime = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::ReadWrite);

    if(indexes.at(0).isValid())
    {
        stream << reinterpret_cast<TreeItem*>(indexes.at(0).internalPointer())->data(0).toString();
    }

    mime->setData("application/treeitem.str", encodedData);
    return mime;
}

