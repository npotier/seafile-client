#ifndef SEAFILE_NETWORK_TASK_BUILDER_H
#define SEAFILE_NETWORK_TASK_BUILDER_H

#include <QString>
class Account;
class SeafileNetworkTask;
class SeafileDownloadTask;

class SeafileNetworkTaskBuilder
{
public:
    SeafileNetworkTaskBuilder() {}
    SeafileDownloadTask* createDownloadTask(const Account &account,
                                           const QString &repo_id,
                                           const QString &path,
                                           const QString &filename,
                                           const QString &download_location);
    SeafileDownloadTask* createDownloadTask(const Account &account,
                                           const QString &repo_id,
                                           const QString &path,
                                           const QString &filename,
                                           const QString &revision,
                                           const QString &download_location);
};












#endif // SEAFILE_NETWORK_TASK_BUILDER_H
