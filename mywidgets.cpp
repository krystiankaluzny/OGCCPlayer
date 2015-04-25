#include "mywidgets.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QDebug>

MyTreeView::MyTreeView(QWidget *parent) :
    QTreeView(parent)
{
}

QModelIndex MyTreeView::firstSelectedIndex() const
{
    QModelIndexList list = QTreeView::selectedIndexes();
    if(list.isEmpty()) return QModelIndex();
    return list.at(0);
}


void MyTreeView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    //QTreeView::drawBranches(painter, rect, index);
}

MyListView::MyListView(QWidget *parent)
    : QListView(parent)
{

}

QModelIndex MyListView::firstSelectedIndex() const
{
    QModelIndexList list = QListView::selectedIndexes();
    if(list.isEmpty()) return QModelIndex();
    return list.at(0);
}

MySlider::MySlider(QWidget *parent):
    QSlider(parent)
{
}

void MySlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        int newVal;
        if (orientation() == Qt::Vertical)
            newVal = minimum() + ((maximum()-minimum()) * (height()-event->y())) / height();
        else
            newVal = minimum() + ((maximum()-minimum()) * event->x()) / width();

        if (invertedAppearance() == true)
            setValue(maximum() - newVal );
        else
            setValue(newVal);

        event->accept();
    }
    QSlider::mousePressEvent(event);
}


NewNameDialog::NewNameDialog(QString name, QWidget *parent)
    :QDialog(parent)
{
    setWindowTitle("Nazwa");
    m_label = new QLabel("Nazwa playlisty:");
    m_line_edit = new QLineEdit(name);
    m_dialog_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_dialog_box->button(QDialogButtonBox::Ok)->setText("OK");
    m_dialog_box->button(QDialogButtonBox::Cancel)->setText("Anuluj");
    connect(m_dialog_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_dialog_box, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(m_label);
    layout->addWidget(m_line_edit);
    layout->addWidget(m_dialog_box);
    setLayout(layout);
}

NewNameDialog::~NewNameDialog()
{
    delete m_line_edit;
    delete m_label;
}

QString NewNameDialog::getName() const
{
    return m_line_edit->text();
}


MySplitter::MySplitter(QWidget *parent)
    : QSplitter(parent)
{

}




