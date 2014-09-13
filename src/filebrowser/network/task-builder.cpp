#include "task-builder.h"
#include "task.h"
#include "account.h"

namespace {
const char *kGetFileUrl = "api2/repos/%1/file/";
const char *kGetFileFromRevisionUrl = "api2/repos/%1/file/revision/";
}

SeafileDownloadTask* SeafileNetworkTaskBuilder::createDownloadTask(
                                       const Account &account,
                                       const QString &repo_id,
                                       const QString &path,
                                       const QString &filename,
                                       const QString &download_location)
{
    QUrl url = account.getAbsoluteUrl(QString(kGetFileUrl).arg(repo_id));
    url.addQueryItem("p", path + filename );
    return new SeafileDownloadTask(account.token, url, filename, download_location);
}

SeafileDownloadTask* SeafileNetworkTaskBuilder::createDownloadTask(
                                       const Account &account,
                                       const QString &repo_id,
                                       const QString &path,
                                       const QString &filename,
                                       const QString &revision,
                                       const QString &download_location)
{
    QUrl url = account.getAbsoluteUrl(QString(kGetFileFromRevisionUrl).arg(repo_id));

    url.addQueryItem("p", path + filename );
    url.addQueryItem("commit_id", revision);
    return new SeafileDownloadTask(account.token, url, filename, download_location);
}
