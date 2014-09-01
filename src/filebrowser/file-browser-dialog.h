#ifndef SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H

#include <QDialog>
#include <QStack>

#include "api/server-repo.h"

class QToolBar;
class QAction;
class QStackedWidget;
class QLineEdit;
class FileBrowserProgressDialog;

class ApiError;
class FileTableView;
class FileTableModel;
class SeafDirent;
class GetDirentsRequest;
class DataManager;
class FileNetworkManager;

/**
 * This dialog is used when the user clicks on a repo not synced yet.
 *
 * The user can browse the files of this repo. When he clicks on a file, the
 * file would be downloaded to a temporary location. If the user modifies the
 * downloaded file, the new version would be automatically uploaded to the
 * server.
 *
 */
class FileBrowserDialog : public QDialog
{
    Q_OBJECT
public:
    FileBrowserDialog(const ServerRepo& repo, QWidget *parent=0);
    ~FileBrowserDialog();

signals:
    void dirChanged();
    void dirChangedForcely();

private slots:
    void onGetDirentsSuccess(const QList<SeafDirent>& dirents);
    void onGetDirentsFailed(const ApiError& error);
    void onDirChanged(bool forcely = false);
    void onDirChangedForcely();
    void onDirentClicked(const SeafDirent& dirent);
    void onBackwardActionClicked();
    void onForwardActionClicked();
    void onNavigateHomeActionClicked();

private:
    Q_DISABLE_COPY(FileBrowserDialog)

    void createToolBar();
    void createFileTable();
    void createLoadingFailedView();

    void onDirClicked(const SeafDirent& dirent);
    void onFileClicked(const SeafDirent& dirent);

    ServerRepo repo_;
    // current path
    QString path_;

    QToolBar *toolbar_;
    QAction *refresh_action_;
    QAction *backward_action_;
    QAction *forward_action_;
    QLineEdit *path_line_edit_;
    QAction *navigate_home_action_;

    QStackedWidget *stack_;
    QWidget *loading_view_;
    QWidget *loading_failed_view_;
    FileTableView *table_view_;
    FileTableModel *table_model_;
    FileBrowserProgressDialog *file_progress_dialog_;

    QStack<QString> *forward_history_;
    QStack<QString> *backward_history_;
    DataManager *data_mgr_;
    FileNetworkManager *file_network_mgr_;
};


#endif  // SEAFILE_CLIENT_FILE_BROWSER_DIALOG_H
