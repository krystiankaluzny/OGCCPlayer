
#ifndef MYTREEVIEW_H
#define MYTREEVIEW_H

#include <QTreeView>
#include "treeitem.h"

class MyTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit MyTreeView(QWidget *parent = 0);
    QModelIndex firstSelectedIndex() const;

protected:
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const;

};

#endif // MYTREEVIEW_H


#ifndef MYLISTVIEW_H
#define MYLISTVIEW_H

#include <QListView>
#include "treeitem.h"

class MyListView : public QListView
{
    Q_OBJECT
public:
    explicit MyListView(QWidget *parent = 0);
    QModelIndex firstSelectedIndex() const;

};

#endif // MYLISTVIEW_H


#ifndef MYSLIDER_H
#define MYSLIDER_H

#include <QSlider>

class MySlider : public QSlider
{
    Q_OBJECT
public:
    explicit MySlider(QWidget *parent = 0);
    void mousePressEvent(QMouseEvent *event);
};

#endif // MYSLIDER_H

#ifndef NEWNAMEDIALOG_H
#define NEWNAMEDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>

class NewNameDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewNameDialog(QString name, QWidget *parent = 0);
    ~NewNameDialog();
    QString getName() const;

private:
    QLineEdit* m_line_edit;
    QLabel* m_label;
    QDialogButtonBox* m_dialog_box;
};

#endif //NEWNAMEDIALOG_H


#ifndef MYSPLITTER_H
#define MYSPLITTER_H

#include <QSplitter>

class MySplitter : public QSplitter
{
    Q_OBJECT
public:
    MySplitter(QWidget* parent = 0);
};

#endif //MYSPLITTER_H
