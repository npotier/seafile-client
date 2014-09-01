#include "file-network-mgr.h"

#include <QThread>
#include <QDebug>
#include <cassert>
#include "utils/utils.h"
#include "api/api-error.h"
#include "network/task.h"

FileNetworkManager::FileNetworkManager(const Account &account)
    : file_cache_dir_(defaultFileCachePath(true))
{
    account_ = account;
    worker_thread_ = new QThread;
    // omg here is a ghost story
    assert(file_cache_dir_.exists());
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
                                           const QString &file_name) {
    //TODO: solve conflict in path
    file_cache_dir_.mkpath(repo_id + path);

    QString file_location = file_cache_dir_.absoluteFilePath(repo_id + path);

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
    connect(ftask, SIGNAL(run()), task, SLOT(onRun()));
    connect(ftask, SIGNAL(cancel()), task, SLOT(onCancel()));

    connect(task, SIGNAL(started()), ftask, SLOT(onStarted()));
    connect(task, SIGNAL(redirected(bool)), ftask, SLOT(onRedirected(bool)));
    connect(task, SIGNAL(updateProgress(qint64, qint64)),
            ftask, SLOT(onUpdateProgress(qint64, qint64)));
    connect(task, SIGNAL(aborted()), ftask, SLOT(onAborted()));
    connect(task, SIGNAL(finished(const QString &)), ftask, SLOT(onFinished(const QString &)));

    (*ftask).status = SEAFILE_NETWORK_TASK_STATUS_FRESH;
    worker_thread_->start();
    emit run(num);
    return num;
}
