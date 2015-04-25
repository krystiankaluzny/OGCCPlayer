#ifndef MYLISTMODEL_H
#define MYLISTMODEL_H

#include "mytreemodel.h"

class MyListModel : public MyTreeModel
{
    Q_OBJECT
public:
    explicit MyListModel(const QStringList &headers, QStringList data = QStringList(), int column_count = 0, QObject *parent = 0);

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

};

#endif // MYLISTMODEL_H
