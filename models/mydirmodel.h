#ifndef MYDIRMODEL_H
#define MYDIRMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include "treeitem.h"

class MyDirModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    MyDirModel(const QStringList &headers = QStringList(), QStringList data = QStringList(), int column_count = 0, QObject *parent = 0);
     ~MyDirModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole);
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    QStringList mimeTypes() const;
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QMimeData* mimeData(const QModelIndexList &indexes) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual Qt::DropActions supportedDragActions() const;    

    void setColumnCount(int count);
    void deleteEnable(bool can);
    TreeItem* appendNewTree(QString& path, bool create_audio = false);
    TreeItem* insertNewTree(int position, QString& path, const QModelIndex& parent = QModelIndex(), bool create_audio = false);
    QList<TreeItem *> getTrees() const;
    QList<TreeItem* > getAllTrees(bool without_dirs = true) const;
    virtual void deleteChildren();

protected:
    TreeItem *getItem(const QModelIndex& index) const;

    TreeItem* rootItem;
    bool m_delete_enable; //należy ustawić na fałsz jeżeli drzewa są tworzone na zewnątrz modelu
    int m_column_count;

};

#endif // MYDIRMODEL_H
