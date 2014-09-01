#ifndef SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
#define SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
#include <QProgressDialog>
#include "file-network-mgr.h"
class QProgressBar;

class FileBrowserProgressDialog : public QProgressDialog {
    Q_OBJECT;
    const FileNetworkTask *task_;
public slots:
    void onStarted();
    void onUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void onAborted();
    void onFinished(const QString &file_path);
public:
    FileBrowserProgressDialog(QWidget *parent = 0);
    void setTask(const FileNetworkTask *task);
};



#endif // SEAFILE_CLIENT_FILE_BROWSER_PROGRESS_DIALOG_H
