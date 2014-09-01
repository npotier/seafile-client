#ifndef SEAFILE_NETWORK_TASK_H
#define SEAFILE_NETWORK_TASK_H

#include <QSharedPointer>
#include "request.h"
class QNetworkReply;
class QSslError;
class QFile;
class QNetworkAccessManager;

typedef enum {
    SEAFILE_NETWORK_TASK_UNKNOWN,
    SEAFILE_NETWORK_TASK_DOWNLOAD,
    SEAFILE_NETWORK_TASK_UPLOAD,
} SeafileNetworkTaskType;

typedef enum {
    SEAFILE_NETWORK_TASK_STATUS_UNKNOWN,
    SEAFILE_NETWORK_TASK_STATUS_FRESH,
    SEAFILE_NETWORK_TASK_STATUS_PROCESSING,
    SEAFILE_NETWORK_TASK_STATUS_REDIRECTING,
    SEAFILE_NETWORK_TASK_STATUS_FINISHED,
    SEAFILE_NETWORK_TASK_STATUS_CANCELING,
    SEAFILE_NETWORK_TASK_STATUS_ABORTED,
    SEAFILE_NETWORK_TASK_STATUS_ERROR,
} SeafileNetworkTaskStatus;

typedef enum {
    SEAFILE_NETWORK_TASK_UNKNOWN_ERROR,
    SEAFILE_NETWORK_TASK_NO_ERROR,
    SEAFILE_NETWORK_TASK_FILE_ERROR,
    SEAFILE_NETWORK_TASK_NETWORK_ERROR,
} SeafileNetworkTaskError;

class SeafileNetworkTask : public QObject {
    Q_OBJECT
public:
    SeafileNetworkTask(const QString &token, const QUrl &url);
    ~SeafileNetworkTask();

    SeafileNetworkTaskStatus status() { return status_; }

    QUrl url() { return url_; }
    void setUrl(const QUrl &url) { url_ = url; }

public slots:
    void cancel() {}

protected slots:
#ifndef QT_NO_OPENSSL
    void sslErrors(QNetworkReply*, const QList<QSslError> &errors);
#endif

protected:
    void createNetworkAccessManger();
    void startRequest();
    QNetworkReply *reply_;
    SeafileNetworkRequest *req_;
    QSharedPointer<QNetworkAccessManager> network_mgr_;

    SeafileNetworkTaskType type_;
    SeafileNetworkTaskStatus status_;
    QString token_;
    QUrl url_;
};

class SeafileDownloadTask : public SeafileNetworkTask {
    Q_OBJECT
public:
    SeafileDownloadTask(const QString &token,
                        const QUrl &url,
                        const QString &file_name,
                        const QString &download_location,
                        bool auto_redirect = true);
    ~SeafileDownloadTask();

    QString file_name() { return file_name_; }
    void setFilename(const QString &file_name) { file_name_ = file_name; }
    QString downloadLocation() { return download_location_; }
    void setDownloadLocation(const QString &download_location) {
        download_location_ = download_location;
    }

signals:
    void started();
    void redirected(bool auto_redirect);
    void updateProgress(qint64 processed_bytes, qint64 total_bytes);
    void aborted();
    void finished(const QString &file_name);

public slots:
    void onRun();
    void onCancel();

private slots:
    void httpUpdateProgress(qint64 processed_bytes, qint64 total_bytes);
    void httpProcessReady();
    void httpFinished();
    void onRedirected(const QUrl &new_url, bool auto_redirect = true);
    void onAborted(SeafileNetworkTaskError error = SEAFILE_NETWORK_TASK_UNKNOWN_ERROR);

private:
    void startRequest();

    bool auto_redirect_;
    QFile *file_;
    QByteArray buf_;
    QString file_name_;
    QString download_location_;
};
#endif // SEAFILE_NETWORK_TASK_H
