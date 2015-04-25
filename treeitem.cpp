#include "treeitem.h"
#include <QFileInfo>
#include <QStringList>
#include <QDateTime>
#include <QDir>
#include <QDebug>

#include <bass.h>

TreeItem::TreeItem(const QVector<QVariant> &data, bool is_dir, TreeItem *parent)
    : is_dir(is_dir),
      itemData(data),
      parentItem(parent)
{
}

TreeItem::~TreeItem()
{
    removeAllChildren();
}

TreeItem *TreeItem::child(int row)
{
    return childItems.value(row);
}

QList<TreeItem *> TreeItem::getChildren() const
{
    return childItems;
}

QList<TreeItem *> TreeItem::getAllChildren(bool without_dirs) const
{
    QList<TreeItem* > all;
    foreach (TreeItem* item, childItems)
    {
        if(item->isDir())
        {
            if(!without_dirs) all.append(item);
            all.append(item->getAllChildren(without_dirs));
        }
        else
            all.append(item);
    }

    return all;
}

int TreeItem::childCount() const
{
    return childItems.size();
}

int TreeItem::columnCount() const
{
    return itemData.size();
}

QVariant TreeItem::data(int column) const
{
    //"Full Path" << "Name" << "Last Modification" << "Duration" << "Play cont"<< "Exist"
    return itemData.value(column); //Zwraca domyślny konstruktor jeżli column jest < 0 lub >= size
}

void TreeItem::setParent(TreeItem *parent)
{
    parentItem = parent;
}

int TreeItem::positionOfThis() const
{
    if(parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
    return -1;
}

TreeItem *TreeItem::parent() const
{
    return parentItem;
}

bool TreeItem::insertChildren(int position, int count, int columns)
{
    if(position < 0 || position > childItems.size())
        return false;

    for(int i = 0; i < count; i++)
    {
        QVector<QVariant> data(columns);
        TreeItem* item = new TreeItem(data, this);
        childItems.insert(position, item);
    }
    return true;
}

bool TreeItem::insertChild(int position, TreeItem *child)
{
    if(child == nullptr) return false;
    child->parentItem = this;
    if(position < childCount() && position >= 0)
        childItems.insert(position, child);
    else
        childItems.append(child);

    return true;
}

bool TreeItem::moveChild(int position, int destiny)
{
    if(position < 0 || position >= childCount()) return false;
    TreeItem* item = childItems.takeAt(position);
    return insertChild(destiny, item);
}

bool TreeItem::removeChildren(int position, int count)
{
    if(position < 0)
        return false;

    if(position + count > childItems.size())
        count = childItems.size() - position;

    for(int i = 0; i < count; i++)
    {
        delete childItems.takeAt(position); // usuń z listy a następnie usuń zawartość
    }
    return true;
}

void TreeItem::removeAllChildren()
{
    QList<TreeItem*>::iterator iter;
    for(iter = childItems.begin(); iter != childItems.end(); iter++)
        delete (*iter);
    childItems.clear();
}

bool TreeItem::removeChild(QList<TreeItem*>::iterator iter)
{
    return removeChildren(childItems.indexOf(*iter), 1);
}

bool TreeItem::clearChildren(int position, int count)
{
    if(position < 0 || position + count > childItems.size())
        return false;

    for(int i = 0; i < count; i++)
    {
        childItems.takeAt(position); // usuń z listy
    }
    return true;
}

void TreeItem::clearAllChildren()
{
    QList<TreeItem*>::iterator iter;
    for(iter = childItems.begin(); iter != childItems.end(); iter++)
        (*iter)->setParent(nullptr);
    childItems.clear(); //czyszczenie listy ale nie usuwanie obiektów
}

bool TreeItem::insertColumns(int position, int columns)
{
    if(position < 0 || position > itemData.size())
        return false;

    for(int i = 0; i < columns; i++)
        itemData.insert(position, QVariant());

    for(TreeItem* child : childItems)
        child->insertColumns(position, columns);

    return true;
}

bool TreeItem::removeColumns(int position, int columns)
{
    if(position < 0 || position + columns > itemData.size())
        return false;

    for(int i = 0; i < columns; i++)
        itemData.remove(position);

    for(TreeItem* child : childItems)
        child->removeColumns(position, columns);

    return true;
}

bool TreeItem::setData(int column, const QVariant &value)
{
    if(column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;

    return true;
}

bool TreeItem::setDownTree(int column, const QVariant &value)
{
    int result = !setData(column, value);

    QList<TreeItem*>::iterator iter;
    for(iter = childItems.begin(); iter != childItems.end(); iter++)
        result += !(*iter)->setDownTree(column, value);

    return !result;
}

void TreeItem::setDir(bool dir)
{
    is_dir = dir;
}

bool TreeItem::isDir() const
{
    return is_dir;
}

QVector<int> TreeItem::getPath()
{
    QVector<int> path;
    if(parentItem != nullptr)
    {
        path << parentItem->getPath();
        path << positionOfThis();
    }
    return path;
}

TreeItem* createDirTree(int position, const QString &dir, TreeItem *parent)
{
    QFileInfo fileInfo(dir);

    if(fileInfo.isDir())
    {
        QVector<QVariant> data;
        data << fileInfo.absoluteFilePath() << fileInfo.fileName() << fileInfo.lastModified();

        TreeItem* tree = new TreeItem(data, true, parent);

        QDir d(dir);
        QFileInfoList list = d.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDot | QDir::NoDotDot, QDir::Name | QDir::DirsFirst);

        for(int j = 0; j < list.size(); j++)
            createDirTree(-1, list.at(j).absoluteFilePath(), tree);

        if(parent != nullptr)
            parent->insertChild(position, tree);

        return tree;
    }
    else if(fileInfo.isFile())
    {
        QVector<QVariant> data;
        data << fileInfo.absoluteFilePath() << fileInfo.fileName() << fileInfo.lastModified();

        TreeItem* tree = new TreeItem(data, false, parent);

        if(parent != nullptr)
            parent->insertChild(position, tree);

        return tree;
    }

    return nullptr;
}


TreeItem *createAudioMediaTree(int position, const QString &dir, TreeItem *parent)
{
    QFileInfo fileInfo(dir);
    static const QStringList extension(QStringList() << "mp3" << "MP3" << "ogg" << "OGG");

    if(fileInfo.isDir())
    {
        QVector<QVariant> data;
        data << fileInfo.absoluteFilePath() << fileInfo.fileName() << QVariant(static_cast<unsigned long long>(0)) << QVariant("") << QVariant(fileInfo.exists());
        TreeItem* tree = new TreeItem(data, true, parent);

        QDir d(dir);
        QFileInfoList list = d.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDot | QDir::NoDotDot, QDir::Name | QDir::DirsFirst);

        for(int j = 0; j < list.size(); j++)
            createAudioMediaTree(-1, list.at(j).absoluteFilePath(), tree);

        if(parent != nullptr)
        {
            parent->insertChild(position, tree);
            if(parent->parent() != nullptr)
                parent->setData(2, QVariant(static_cast<unsigned long long>(parent->data(2).toULongLong() + tree->data(2).toULongLong())));
        }


        return tree;
    }
    else if(fileInfo.isFile())
    {
        if(extension.contains(fileInfo.suffix()))
        {
            bool exists = fileInfo.exists();

            QVector<QVariant> data;
            data << fileInfo.absoluteFilePath() << fileInfo.fileName() << QVariant(static_cast<unsigned long long>(0)) << QVariant(static_cast<unsigned long long>(0)) << QVariant(exists);

            TreeItem* tree = new TreeItem(data, false, parent);

            if(parent != nullptr)
                parent->insertChild(position, tree);

            if(exists)
            {
                HSTREAM str = BASS_StreamCreateFile(FALSE, tree->data(0).toString().toStdString().c_str(), 0, 0, 0);
                if(str)
                {
                    QWORD length = BASS_ChannelGetLength(str, BASS_POS_BYTE);
                    double duration = BASS_ChannelBytes2Seconds(str, length);
                    tree->setData(2, QVariant(static_cast<unsigned long long>(duration * 1000)));
                    if(parent->parent() != nullptr)
                        parent->setData(2, QVariant(static_cast<unsigned long long>(parent->data(2).toULongLong() + duration * 1000)));
                    BASS_StreamFree(str);
                }
            }
            return tree;
        }
    }

    return nullptr;
}

bool moveItem(TreeItem *sourceParent, int source, TreeItem *destinyParent, int destiny)
{
    if(source < 0 || source >= sourceParent->childCount()) return false;
    TreeItem* s = sourceParent->childItems.takeAt(source);
    if(s == destinyParent) return false;
    destinyParent->insertChild(destiny, s);
    return true;
}
