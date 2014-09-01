#include "file-browser-progress-dialog.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QDebug>

FileBrowserProgressDialog::FileBrowserProgressDialog(QWidget *parent)
        : QProgressDialog(parent), task_(NULL) {}

void FileBrowserProgressDialog::setTask(const FileNetworkTask *task)
{
    task_ = task;
    setLabelText(((task->type == SEAFILE_NETWORK_TASK_UPLOAD) ?
       tr("Uploading %1") : tr("Downloading %1")).arg(task->file_name));

    connect(task_, SIGNAL(started()), this, SLOT(onStarted()));
    connect(task_, SIGNAL(updateProgress(qint64, qint64)),
            this, SLOT(onUpdateProgress(qint64, qint64)));
    connect(task_, SIGNAL(aborted()), this, SLOT(onAborted()));
    connect(task_, SIGNAL(finished(const QString &)), this, SLOT(onFinished(const QString &)));
    connect(this, SIGNAL(canceled()), task_, SLOT(onCancel()));
}

void FileBrowserProgressDialog::onStarted()
{
    setMaximum(0);
    setValue(0);
    setLabelText(labelText().append(" ... "));
}
void FileBrowserProgressDialog::onUpdateProgress(qint64 processed_bytes, qint64 total_bytes)
{
    if (maximum() != total_bytes)
        setMaximum(total_bytes);
    setValue(processed_bytes);
}
void FileBrowserProgressDialog::onAborted()
{
    disconnect(task_, 0, this, 0);
    setLabelText(labelText().append("aborted"));
    QMessageBox::warning(static_cast<QWidget*>(parent()), tr("Aborted"), labelText());
}
void FileBrowserProgressDialog::onFinished(const QString &file_path)
{
    disconnect(task_, 0, this, 0);
    setLabelText(labelText().append("finished"));
    setValue(maximum());
    QDesktopServices::openUrl(QUrl::fromLocalFile(file_path));
}
