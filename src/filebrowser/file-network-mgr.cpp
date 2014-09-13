#include "file-network-mgr.h"

#include <QThread>
#include <QDebug>
#include <cassert>
#include "utils/utils.h"
#include "api/api-error.h"
#include "network/task.h"

FileNetworkManager::FileNetworkManager(const Account &account)
    : file_cache_dir_(defaultDownloadsPath()),
    file_cache_path_(defaultDownloadsPath())
{
    if (!file_cache_path_.endsWith("/"))
        file_cache_path_.append('/');
    account_ = account;
    worker_thread_ = new QThread;
}

FileNetworkManager::~FileNetworkManager()
{
    worker_thread_->quit();
    worker_thread_->wait();
    delete worker_thread_;
    while (!tasks_.isEmpty())
    {
        FileNetworkTask* last = tasks_.last();
        tasks_.pop_back();
        delete last;
    }

}

int FileNetworkManager::createDownloadTask(const QString &repo_id,
                                           const QString &path,
                                           const QString &file_name)
{
    QString file_location(file_cache_dir_.absoluteFilePath(file_name));
    FileNetworkTask* ftask = \
           new FileNetworkTask(repo_id, path, file_name, file_location);
    const int num = addTask(ftask);
    (*ftask).task_num = num;
    (*ftask).type = SEAFILE_NETWORK_TASK_DOWNLOAD;

    SeafileDownloadTask* task = \
      builder_.createDownloadTask(account_, repo_id,
                                  path, file_name, file_location);

    task->moveToThread(worker_thread_);
    connect(worker_thread_, SIGNAL(finished()), ftask, SLOT(onCancel()));

    connect(this, SIGNAL(run(int)), ftask, SLOT(onRun(int)));
    connect(ftask, SIGNAL(start()), task, SIGNAL(start()));
    connect(ftask, SIGNAL(cancel()), task, SIGNAL(cancel()));

    connect(task, SIGNAL(started()), ftask, SLOT(onStarted()));
    connect(task, SIGNAL(updateProgress(qint64, qint64)),
            ftask, SLOT(onUpdateProgress(qint64, qint64)));
    connect(task, SIGNAL(aborted()), ftask, SLOT(onAborted()));
    connect(task, SIGNAL(finished(const QString &)),
            ftask, SLOT(onFinished(const QString &)));

    (*ftask).status = SEAFILE_NETWORK_TASK_STATUS_FRESH;
    worker_thread_->start();
    emit run(num);
    return num;
}


int FileNetworkManager::createUploadTask(const QString &repo_id,
                                           const QString &path,
                                           const QString &file_name,
                                           const QString &update_file_path)
{
    QString file_location(QFileInfo(update_file_path).absolutePath());
    FileNetworkTask* ftask = \
           new FileNetworkTask(repo_id, path, file_name, file_location);
    const int num = addTask(ftask);
    (*ftask).task_num = num;
    (*ftask).type = SEAFILE_NETWORK_TASK_UPLOAD;

    SeafileUploadTask* task = \
      builder_.createUploadTask(account_, repo_id,
                                  path, file_name, file_location);

    task->moveToThread(worker_thread_);
    connect(worker_thread_, SIGNAL(finished()), ftask, SLOT(onCancel()));

    connect(this, SIGNAL(run(int)), ftask, SLOT(onRun(int)));
    connect(ftask, SIGNAL(start()), task, SIGNAL(start()));
    connect(ftask, SIGNAL(cancel()), task, SIGNAL(cancel()));

    connect(task, SIGNAL(started()), ftask, SLOT(onStarted()));
    connect(task, SIGNAL(updateProgress(qint64, qint64)),
            ftask, SLOT(onUpdateProgress(qint64, qint64)));
    connect(task, SIGNAL(aborted()), ftask, SLOT(onAborted()));
    connect(task, SIGNAL(finished(const QString &)),
            ftask, SLOT(onFinished(const QString &)));

    (*ftask).status = SEAFILE_NETWORK_TASK_STATUS_FRESH;
    worker_thread_->start();
    emit run(num);
    return num;
}
