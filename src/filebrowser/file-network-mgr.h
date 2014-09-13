#ifndef SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
#define SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H

#include <QList>
#include <QDir>

#include "account.h"
#include "network/task-builder.h"
#include "network/task.h"

class ApiError;
class QThread;
class FileNetworkTask : public QObject {
    Q_OBJECT;

signals:
    void start();
    void cancel();

    void started();
    void updateProgress(qint64 processed_bytes_, qint64 total_bytes_);
    void aborted();
    void finished();
private slots:
    inline void onRun(int num);
    inline void onCancel();

    inline void onStarted();
    inline void onUpdateProgress(qint64 processed_bytes_, qint64 total_bytes_);
    inline void onAborted();
    inline void onFinished(const QString &file_real_path_);

public:
  FileNetworkTask(const QString &repo_id_,
                  const QString &path_,
                  const QString &file_name_,
                  const QString &file_location_)
    : task_num(-1),
    repo_id(repo_id_),
    path(path_),
    file_name(file_name_),
    file_location(file_location_),
    file_real_path(file_location_ + "/" + file_name_),
    processed_bytes(0), total_bytes(0),
    status(SEAFILE_NETWORK_TASK_STATUS_UNKNOWN),
    type(SEAFILE_NETWORK_TASK_UNKNOWN) {}
    int task_num;
    QString repo_id;
    QString path;
    QString file_name;
    QString file_location;
    QString file_real_path;
    qint64 processed_bytes;
    qint64 total_bytes;
    SeafileNetworkTaskStatus status;
    SeafileNetworkTaskType type;
};

class FileNetworkManager : public QObject {
    Q_OBJECT
public:
    FileNetworkManager(const Account &account);

    int createDownloadTask(const QString &repo_id,
                               const QString &path,
                               const QString &filename);
    int createDownloadTask(const QString &repo_id,
                               const QString &path,
                               const QString &filename,
                               const QString &revision);

    FileNetworkTask* getTask(int num) {
        return tasks_[num];
    }

    ~FileNetworkManager();

signals:
    void run(int num);

private:
    inline int addTask(FileNetworkTask* task);

    Account account_;

    QDir file_cache_dir_;
    QString file_cache_path_;

    QList<FileNetworkTask*> tasks_;

    SeafileNetworkTaskBuilder builder_;

    QThread *worker_thread_;
};

void FileNetworkTask::onRun(int num)
{
    if(num == task_num)
        emit start();
}

void FileNetworkTask::onCancel()
{
    status = SEAFILE_NETWORK_TASK_STATUS_CANCELING;
    emit cancel();
}

void FileNetworkTask::onStarted()
{
    status = SEAFILE_NETWORK_TASK_STATUS_PROCESSING;
    emit started();
}

void FileNetworkTask::onUpdateProgress(qint64 processed_bytes_,
                                            qint64 total_bytes_)
{
    processed_bytes = processed_bytes_;
    total_bytes = total_bytes_;
    emit updateProgress(processed_bytes, total_bytes);
}

void FileNetworkTask::onAborted()
{
    status = SEAFILE_NETWORK_TASK_STATUS_ABORTED;
    emit aborted();
}

void FileNetworkTask::onFinished(const QString &file_real_path_)
{
    file_real_path = file_real_path_;
    status = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
    emit finished();
}

int FileNetworkManager::addTask(FileNetworkTask* task)
{
    tasks_.push_back(task);
    return tasks_.size() - 1;
}


#endif //SEAFILE_CLIENT_FILE_BROWSER_NETWORK_MANAGER_H
