#ifndef TREEITEM_H
#define TREEITEM_H
#include <QList>
#include <QVector>
#include <QVariant>

class TreeItem
{
public:
    TreeItem(const QVector<QVariant> &data, bool is_dir = false, TreeItem *parent = nullptr);
    ~TreeItem();

    TreeItem* child(int row);
    QList<TreeItem* > getChildren() const;
    QList<TreeItem* > getAllChildren(bool without_dirs = true) const;

    int childCount() const; //==row count
    int columnCount() const;
    QVariant data(int column) const;
    TreeItem* parent() const;
    void setParent(TreeItem* parent);
    bool insertChildren(int position, int count, int columns); //insert empty trees
    bool insertChild(int position, TreeItem* child); // position = -1 to append
    bool moveChild(int position, int destiny);
    bool removeChildren(int position, int count);
    void removeAllChildren();
    bool removeChild(QList<TreeItem*>::iterator iter);
    bool clearChildren(int position, int count);
    void clearAllChildren(); //usuwanie wskaźników z listy ale nie usuwanie obiektów
    bool insertColumns(int position, int columns);
    bool removeColumns(int position, int columns);
    int positionOfThis() const;
    bool setData(int column, const QVariant& value);
    bool setDownTree(int column, const QVariant &value);
    void setDir(bool dir);
    bool isDir() const;
    QVector<int> getPath();

private:
    bool is_dir;
    QList<TreeItem*> childItems;
    QVector<QVariant> itemData;
    TreeItem* parentItem;

    friend bool moveItem(TreeItem* ,int ,TreeItem* ,int );
};

TreeItem* createDirTree(int position, const QString &dir, TreeItem *parent);
TreeItem* createAudioMediaTree(int position, const QString &dir, TreeItem *parent);
bool moveItem(TreeItem* sourceParent, int source, TreeItem* destinyParent, int destiny);
#endif // TREEITEM_H
