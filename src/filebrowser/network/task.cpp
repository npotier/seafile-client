#include "task.h"

#include <QDebug>
#include <QDir>
#include <QTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include "utils/utils.h"

SeafileNetworkTask::SeafileNetworkTask(const QString &token,
                                       const QUrl &url)
    :token_(token), url_(url)
{
}
#ifndef QT_NO_OPENSSL
void SeafileNetworkTask::sslErrors(QNetworkReply*, const QList<QSslError> &errors)
{
    qWarning() << dumpSslErrors(errors);

    //TODO
    reply_->ignoreSslErrors();
}
#endif
SeafileNetworkTask::~SeafileNetworkTask()
{
    delete req_;
}
void SeafileNetworkTask::createNetworkAccessManger()
{
    network_mgr_ = QSharedPointer<QNetworkAccessManager>(new QNetworkAccessManager());
#ifndef QT_NO_OPENSSL
    connect(network_mgr_.data(), SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)),
             this, SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));
#endif
}

SeafileDownloadTask::SeafileDownloadTask(const QString &token,
                                         const QUrl &url,
                                         const QString &file_name,
                                         const QString &download_location,
                                         bool auto_redirect)
    :SeafileNetworkTask(token, url), auto_redirect_(auto_redirect),
    file_(NULL), file_name_(file_name), download_location_(download_location)
{
    type_ = SEAFILE_NETWORK_TASK_DOWNLOAD;
    status_ = SEAFILE_NETWORK_TASK_STATUS_FRESH;
    reply_ = NULL;
    req_ = new SeafileNetworkRequest(token_, url_);
}

SeafileDownloadTask::~SeafileDownloadTask()
{
    cancel();
    delete file_;
    if (reply_)
        reply_->deleteLater();
}

void SeafileDownloadTask::startRequest()
{
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_PROCESSING)
        return;
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_CANCELING)
        return;
    status_ = SEAFILE_NETWORK_TASK_STATUS_PROCESSING;
    req_->setToken(token_);
    req_->setUrl(url_);
    reply_ = network_mgr_->get(*req_);
    connect(reply_, SIGNAL(finished()), this, SLOT(httpFinished()));
    connect(reply_, SIGNAL(readyRead()), this, SLOT(httpProcessReady()));
    connect(reply_, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(httpUpdateProgress(qint64,qint64)));
}

void SeafileDownloadTask::onRun()
{
    if (network_mgr_.isNull())
        createNetworkAccessManger();
    QDir dir(download_location_);
    //unable to open download location
    if (!dir.exists()) {
        qDebug() << "[download task]" << "location" << download_location_ << "not existed";
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }
    file_ = new QFile(dir.filePath(file_name_));
    if (file_->exists()) {
        qDebug() << "[download task]" << (download_location_ + file_name_)
          << "exists";
        QFileInfo file_info(file_name_);
        QString alternative_file_path(download_location_ + file_info.baseName()
                                      + " (%1)." + file_info.completeSuffix());
        int i;
        for (i = 1; i != 10; i++) {
            file_->setFileName(alternative_file_path.arg(i));
            if (!file_->exists())
                break;
        }
        if (i == 10) {
            qDebug() << "[download task]"
              << (download_location_ + file_name_)
              << "unable to find an alternative file name";
            onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
            return;
        }
    }
    if (!file_->open(QIODevice::WriteOnly)) {
        qDebug() << "[download task]" << (download_location_ + file_name_) << "is unable to open";
        onAborted(SEAFILE_NETWORK_TASK_FILE_ERROR);
        return;
    }
    file_->resize(0);

    emit started();
    startRequest();
}

void SeafileDownloadTask::httpUpdateProgress(qint64 processed_bytes, qint64 total_bytes)
{
    emit updateProgress(processed_bytes, total_bytes);
}

void SeafileDownloadTask::httpProcessReady()
{
    if (auto_redirect_) {
        buf_ += reply_->readAll();
    }
    else if (file_) {
        file_->write(reply_->readAll());
    }
}

void SeafileDownloadTask::httpFinished()
{
    if (status_ == SEAFILE_NETWORK_TASK_STATUS_CANCELING) {
        onAborted(SEAFILE_NETWORK_TASK_NO_ERROR);
        return;
    }
    file_->flush();
    file_->close();
    QVariant redirectionTarget = reply_->attribute(
        QNetworkRequest::RedirectionTargetAttribute);
    if (reply_->error()) {
        onAborted(SEAFILE_NETWORK_TASK_NETWORK_ERROR);
        return;
    } else if (!redirectionTarget.isNull()) {
        status_ = SEAFILE_NETWORK_TASK_STATUS_REDIRECTING;
        QUrl new_url = QUrl(url_).resolved(redirectionTarget.toUrl());
        onRedirected(new_url);
        return;
    } else if (auto_redirect_) {
        status_ = SEAFILE_NETWORK_TASK_STATUS_REDIRECTING;
        QString new_url(buf_);
        if(new_url.size() <= 2) {
            onAborted(SEAFILE_NETWORK_TASK_NETWORK_ERROR);
            return;
        }
        new_url.remove(0, 1);
        new_url.chop(1);
        auto_redirect_ = false;
        onRedirected(new_url, false);
        return;
    } else {
        //onFinished
        qDebug() << "[download task]" << url_.toEncoded() << "\tfinished";
        QString filePath = QFileInfo(*file_).absoluteFilePath();
        status_ = SEAFILE_NETWORK_TASK_STATUS_FINISHED;
        emit finished(filePath);
     }
    reply_->deleteLater();
    reply_ = NULL;
    delete file_;
    file_ = NULL;
}

void SeafileDownloadTask::onCancel()
{
    qDebug() << "[download task]" << url_.toEncoded() << " is cancelling";
    if (reply_ && status_ == SEAFILE_NETWORK_TASK_STATUS_PROCESSING) {
        reply_->abort();
        status_ = SEAFILE_NETWORK_TASK_STATUS_CANCELING;
    }
}
void SeafileDownloadTask::onRedirected(const QUrl &new_url, bool auto_redirect)
{
    if(!new_url.isValid()) {
        onAborted(SEAFILE_NETWORK_TASK_NETWORK_ERROR);
        return;
    }
    url_ = new_url;
    reply_->deleteLater();
    reply_ = NULL;
    file_->open(QIODevice::WriteOnly);
    file_->resize(0);
    qDebug() << "[download task]" << "redirected to" << url_.toEncoded();
    emit redirected(auto_redirect);
    startRequest();
}

void SeafileDownloadTask::onAborted(SeafileNetworkTaskError error)
{
    qDebug() << "[download task]" << url_.toEncoded()
      << " is aborted due to error " << error;
    status_ = SEAFILE_NETWORK_TASK_STATUS_ABORTED;
    if (file_) {
        file_->close();
        file_->remove();
        delete file_;
        file_ = NULL;
    }
    reply_->deleteLater();
    reply_ = NULL;
    emit aborted();
}
