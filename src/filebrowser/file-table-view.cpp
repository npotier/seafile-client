#include "file-table-view.h"

#include <QtGui>
#include <QMouseEvent>

#include "file-delegate.h"

FileTableView::FileTableView(const ServerRepo& repo, QWidget *parent)
    : QTableView(parent),
      curr_hovered_(-1), // -1 is a publicly-known magic number
      repo_(repo)
{
    verticalHeader()->hide();
    horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setCascadingSectionResizes(true);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setSortIndicatorShown(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setAlternatingRowColors(true);

    setGridStyle(Qt::NoPen);
    setShowGrid(false);

    setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(onItemDoubleClicked(const QModelIndex&)));

    FileDelegate *delegate = new FileDelegate;
    delegate->setView(this);

    setAcceptDrops(true);

    setItemDelegate(delegate);
    setMouseTracking(true);
}

void FileTableView::onItemDoubleClicked(const QModelIndex& index)
{
    FileTableModel *_model = static_cast<FileTableModel*>(model());
    const SeafDirent dirent = _model->direntAt(index.row());

    emit direntClicked(dirent);
}

void FileTableView::setMouseOver(const int row)
{
    if (row == curr_hovered_) return;

    FileTableModel *_model = static_cast<FileTableModel*>(model());
    const SeafDirent dirent = _model->direntAt(row);

    emit direntMouseOver(dirent);

    if (curr_hovered_ != -1)
        disableMouseOver();

    curr_hovered_ = row;
}

void FileTableView::disableMouseOver()
{
    //finished
    FileTableModel *_model = static_cast<FileTableModel*>(model());
    const SeafDirent dirent = _model->direntAt(curr_hovered_);

    emit direntMouseOverDone(dirent);
}

void FileTableView::mouseMoveEvent(QMouseEvent *event)
{

    // TODO: you need know when mouse are not in table rect
    // then you need disable over

    QTableView::mouseMoveEvent(event);
}

QStyleOptionViewItem FileTableView::viewOptions () const
{
    QStyleOptionViewItemV4 option = QTableView::viewOptions();
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option.decorationAlignment = Qt::AlignHCenter | Qt::AlignCenter;
    option.decorationPosition = QStyleOptionViewItem::Top;
    return option;
}

void FileTableView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    qDebug() << event->mimeData()->formats();

    if(urls.isEmpty())
        return;

    foreach(const QUrl &url, urls)
    {
        QString fileName = url.toLocalFile();

        if(fileName.isEmpty())
            return;
        qDebug() << "drop event detected: " << fileName;
    }

    event->acceptProposedAction();
}

void FileTableView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << event->proposedAction() << event->source();
    //only handle external source currently
    if(event->source() != NULL)
        return;
    if(event->mimeData()->hasFormat("text/uri-list")) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}
