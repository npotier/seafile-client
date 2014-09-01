#ifndef FB_TABLE_VIEW_H
#define FB_TABLE_VIEW_H

#include <QTableView>
#include "file-table.h"
#include "file-iview.h"

class FileTableView : public QTableView, public FileIView
{
    Q_OBJECT
public:
    FileTableView(const ServerRepo& repo, QWidget *parent=0);
    void setMouseOver(const int);

signals:
    void direntClicked(const SeafDirent& dirent);
    void direntMouseOver(const SeafDirent& dirent);
    void direntMouseOverDone(const SeafDirent& dirent);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    Q_DISABLE_COPY(FileTableView)

    int curr_hovered_;
    void mouseMoveEvent(QMouseEvent *event);
    void disableMouseOver();

    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);

    QStyleOptionViewItem viewOptions() const;

    ServerRepo repo_;
};



#endif
