#ifndef MYTREEMODEL_H
#define MYTREEMODEL_H

#include "treeitem.h"
#include "mydirmodel.h"

class MyTreeModel : public MyDirModel
{
    Q_OBJECT
public:
    explicit MyTreeModel(const QStringList &headers, QStringList data = QStringList(), int column_count = 0, QObject *parent = 0);
    ~MyTreeModel();

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QMimeData* mimeData(const QModelIndexList &indexes) const;

    TreeItem* getRootItem() const;
    void appendItem(TreeItem* item);
    void insertItem(TreeItem* item, int position, const QModelIndex &parent);
    void moveTree(const QModelIndex &sourceParent, int position, const QModelIndex &destinyParent, int destiny);
    void setCurrentItem(TreeItem* item);
    TreeItem* getCurrentItem() const;
    void deleteChildren();
    void removeAllRowsWithValue(int column, QVariant value);

private:
    TreeItem* m_curent_item;
};

#endif // MYLISTMODEL_H
