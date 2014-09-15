#include "file-browser-dialog.h"

#include <QtGui>
#include <QApplication>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "ui/loading-view.h"
#include "utils/paint-utils.h"
#include "utils/utils.h"
#include "api/api-error.h"

#include "file-table.h"
#include "file-table-view.h"
#include "seaf-dirent.h"
#include "data-mgr.h"
#include "data-mgr.h"

#include "file-network-mgr.h"
#include "network/task.h"
#include "file-browser-progress-dialog.h"

namespace {

enum {
    INDEX_LOADING_VIEW = 0,
    INDEX_TABLE_VIEW,
    INDEX_LOADING_FAILED_VIEW,
};

const char kLoadingFaieldLabelName[] = "loadingFailedText";
const char kCurrentPath[] = "Current Path: %1";
const int kToolBarHeight = 36;
const int kToolBarIconSize = 32;
const int kStatusBarHeight = 24;
const int kStatusBarIconSize = 16;

} // namespace

FileBrowserDialog::FileBrowserDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    path_ = "/";
    forward_history_ = new QStack<QString>();
    backward_history_ = new QStack<QString>();
    selected_dirent_ = NULL;

    const Account& account = seafApplet->accountManager()->currentAccount();
    data_mgr_ = new DataManager(account);
    file_network_mgr_ = new FileNetworkManager(account);
    file_progress_dialog_ = new FileBrowserProgressDialog(this);

    setWindowTitle(tr("File Browser - %1").arg(repo.name));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //don't help
    setWindowModality(Qt::NonModal);

    layout_ = new QVBoxLayout;
    setLayout(layout_);
    layout_->setContentsMargins(0, 6, 0, 0);

    createToolBar();
    createLoadingFailedView();
    createFileTable();
    createStatusBar();

    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    stack_->insertWidget(INDEX_TABLE_VIEW, table_view_);
    stack_->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);
    stack_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    layout_->addWidget(toolbar_);
    layout_->addWidget(stack_);
    layout_->addWidget(status_bar_, 0, Qt::AlignBottom);

    connect(table_view_, SIGNAL(direntClicked(const SeafDirent&)),
            this, SLOT(onDirentClicked(const SeafDirent&)));

    connect(data_mgr_, SIGNAL(getDirentsSuccess(const QList<SeafDirent>&)),
            this, SLOT(onGetDirentsSuccess(const QList<SeafDirent>&)));

    connect(data_mgr_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onGetDirentsFailed(const ApiError&)));

    connect(this, SIGNAL(dirChanged()),
            this, SLOT(onDirChanged()));

    connect(this, SIGNAL(dirChangedForcely()),
            this, SLOT(onDirChangedForcely()));

    emit dirChanged();
}

FileBrowserDialog::~FileBrowserDialog()
{
    delete forward_history_;
    delete backward_history_;
    delete data_mgr_;
    delete file_network_mgr_;
}

void FileBrowserDialog::createToolBar()
{
    toolbar_ = new QToolBar;

    const int w = ::getDPIScaledSize(kToolBarIconSize);
    toolbar_->setIconSize(QSize(w, w));
    toolbar_->setMaximumHeight(kToolBarHeight);
    toolbar_->setMinimumHeight(kToolBarHeight);

    backward_action_ = new QAction(tr("Back"), layout_);
    backward_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack));
    backward_action_->setEnabled(false);
    toolbar_->addAction(backward_action_);
    connect(backward_action_, SIGNAL(triggered()), this, SLOT(onBackwardActionClicked()));

    forward_action_ = new QAction(tr("Forward"), layout_);
    forward_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
    forward_action_->setEnabled(false);
    connect(forward_action_, SIGNAL(triggered()), this, SLOT(onForwardActionClicked()));
    toolbar_->addAction(forward_action_);

    navigate_home_action_ = new QAction(tr("Home"), layout_);
    navigate_home_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon));
    connect(navigate_home_action_, SIGNAL(triggered()), this, SLOT(onNavigateHomeActionClicked()));
    toolbar_->addAction(navigate_home_action_);

    path_line_edit_ = new QLineEdit;
    path_line_edit_->setReadOnly(true);
    path_line_edit_->setText(tr(kCurrentPath).arg("/"));
    path_line_edit_->setAlignment(Qt::AlignHCenter | Qt::AlignLeft);
    path_line_edit_->setMinimumWidth(400);
    path_line_edit_->setMaxLength(100);
    path_line_edit_->setFrame(false);
    toolbar_->addWidget(path_line_edit_);
}

void FileBrowserDialog::createFileTable()
{
    loading_view_ = new LoadingView;
    table_view_ = new FileTableView(repo_);
    table_model_ = new FileTableModel;
    table_view_->setModel(table_model_);
    table_view_->setContentsMargins(0,0,0,0);
}

void FileBrowserDialog::createStatusBar()
{
    status_bar_ = new QToolBar;

    const int w = ::getDPIScaledSize(kStatusBarIconSize);
    status_bar_->setIconSize(QSize(w, w));
    status_bar_->setContentsMargins(12, 0, 12, 6);
    status_bar_->setMaximumHeight(kStatusBarHeight);
    status_bar_->setMinimumHeight(kStatusBarHeight);

    QWidget *spacer1 = new QWidget;
    spacer1->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer1);

    settings_action_ = new QAction(this);
    settings_action_->setIcon(QIcon(":/images/account-settings.png"));
    settings_action_->setEnabled(false);
    status_bar_->addAction(settings_action_);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    status_bar_->addWidget(spacer2);

    upload_action_ = new QAction(this);
    upload_action_->setIcon(QIcon(":/images/plus.png"));
    connect(upload_action_, SIGNAL(triggered()), this, SLOT(onFileUpload()));
    status_bar_->addAction(upload_action_);

    download_action_ = new QAction(this);
    download_action_->setIcon(QIcon(":/images/download.png"));
    connect(download_action_, SIGNAL(triggered()), this, SLOT(onFileDownload()));
    status_bar_->addAction(download_action_);

    details_label_ = new QLabel;
    details_label_->setAlignment(Qt::AlignCenter);
    details_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    status_bar_->addWidget(details_label_);

    refresh_action_ = new QAction(this);
    refresh_action_->setIcon(QIcon(":/images/refresh.png"));
    connect(refresh_action_, SIGNAL(triggered()), this, SIGNAL(dirChangedForcely()));
    status_bar_->addAction(refresh_action_);

    QWidget *spacer3 = new QWidget;
    spacer3->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    status_bar_->addWidget(spacer3);

    open_cache_dir_action_ = new QAction(this);
    open_cache_dir_action_->setIcon(QIcon(":/images/folder-open@2x.png"));
    connect(open_cache_dir_action_, SIGNAL(triggered()), this, SLOT(onOpenCacheDir()));
    status_bar_->addAction(open_cache_dir_action_);

    QWidget *spacer4 = new QWidget;
    spacer4->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    status_bar_->addWidget(spacer4);
}

void FileBrowserDialog::onDirChangedForcely()
{
    onDirChanged(true);
}

void FileBrowserDialog::onDirChanged(bool forcely)
{
    if (!path_.endsWith("/")) {
        path_ += "/";
    }

    stack_->setCurrentIndex(INDEX_LOADING_VIEW);
    data_mgr_->getDirents(repo_.id, path_, forcely);
    path_line_edit_->setText(tr(kCurrentPath).arg(path_));
}

void FileBrowserDialog::onGetDirentsSuccess(const QList<SeafDirent>& dirents)
{
    details_label_->setText(
        tr("%1 files and directories found").arg(dirents.size()));
    table_model_->setDirents(dirents);
    stack_->setCurrentIndex(INDEX_TABLE_VIEW);
}

void FileBrowserDialog::onGetDirentsFailed(const ApiError& error)
{
    Q_UNUSED(error);
    stack_->setCurrentIndex(INDEX_LOADING_FAILED_VIEW);
}

void FileBrowserDialog::createLoadingFailedView()
{
    loading_failed_view_ = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout;
    loading_failed_view_->setLayout(layout);

    QLabel *label = new QLabel;
    label->setObjectName(kLoadingFaieldLabelName);
    QString link = QString("<a style=\"color:#777\" href=\"#\">%1</a>").arg(tr("retry"));
    QString label_text = tr("Failed to get files information<br/>"
                            "Please %1").arg(link);
    label->setText(label_text);
    label->setAlignment(Qt::AlignCenter);

    connect(label, SIGNAL(linkActivated(const QString&)),
            this, SIGNAL(dirChanged()));

    layout->addWidget(label);
}
void FileBrowserDialog::onDirentSelected(const SeafDirent &dirent)
{
    if (selected_dirent_ == &dirent)
        return;

    selected_dirent_ = &dirent;

    if (selected_dirent_->isDir())
        download_action_->setEnabled(false);
    else
        download_action_->setEnabled(true);
}

void FileBrowserDialog::onDirentClicked(const SeafDirent& dirent)
{
    if (dirent.isDir()) {
        onDirInvolved(dirent);
    } else {
        onFileInvolved(dirent);
    }
}

void FileBrowserDialog::onDirInvolved(const SeafDirent& dir)
{
    backward_history_->push(path_);
    if(!backward_action_->isEnabled())
        backward_action_->setEnabled(true);

    path_ += dir.name;

    if (!forward_history_->isEmpty()) {
        if (forward_history_->last() == path_) {
            forward_history_->pop_back();
            if (forward_history_->isEmpty())
                forward_action_->setEnabled(false);
        }
        else {
            forward_history_->clear();
            forward_action_->setEnabled(false);
        }

    }

    emit dirChanged();
}

void FileBrowserDialog::onFileInvolved(const SeafDirent& file)
{
    selected_dirent_ = &file;
    onFileDownload();
}

void FileBrowserDialog::onFileUpload()
{
    QString file_name = QFileDialog::getOpenFileName(this,
                                                 tr("Select file to upload"),
                                                 QDir::home().absolutePath());
    if (file_name.isNull())
        return;
    int task_num = \
        file_network_mgr_->createUploadTask(repo_.id, path_,
                           QFileInfo(file_name).fileName(), file_name);
    const FileNetworkTask* task = file_network_mgr_->getTask(task_num);
    file_progress_dialog_->setTask(task);
    file_progress_dialog_->show();
}

void FileBrowserDialog::onFileDownload()
{
    if (selected_dirent_  == NULL)
        return;
    int task_num = file_network_mgr_->createDownloadTask(repo_.id, path_,
                                                selected_dirent_->name);
    const FileNetworkTask* task = file_network_mgr_->getTask(task_num);
    file_progress_dialog_->setTask(task);
    file_progress_dialog_->show();
}

void FileBrowserDialog::onBackwardActionClicked()
{
    if (backward_history_->isEmpty())
        return;

    forward_history_->push(path_);
    if (!forward_action_->isEnabled())
        forward_action_->setEnabled(true);

    path_ = backward_history_->pop();
    if (backward_history_->isEmpty())
        backward_action_->setEnabled(false);

    emit dirChanged();
}

void FileBrowserDialog::onForwardActionClicked()
{
    if (forward_history_->isEmpty())
        return;

    backward_history_->push(path_);
    if (!backward_action_->isEnabled())
        backward_action_->setEnabled(true);

    path_ = forward_history_->pop();
    if (forward_history_->isEmpty())
        forward_action_->setEnabled(false);

    emit dirChanged();
}

void FileBrowserDialog::onNavigateHomeActionClicked()
{
    if (path_ == "/")
        return;

    backward_history_->push(path_);
    path_ = "/";

    if (!forward_history_->isEmpty()) {
        if (forward_history_->last() == path_) {
            forward_history_->pop_back();
            if (forward_history_->isEmpty())
                forward_action_->setEnabled(false);
        }
        else {
            forward_history_->clear();
            forward_action_->setEnabled(false);
        }

    }

    emit dirChanged();
}

void FileBrowserDialog::onOpenCacheDir()
{
    QDir file_cache_dir_(defaultFileCachePath());
    QString file_location(
        file_cache_dir_.absoluteFilePath(repo_.id + path_));
    if (file_cache_dir_.mkpath(file_location))
        QDesktopServices::openUrl(QUrl::fromLocalFile(file_location));
    else
        QMessageBox::warning(this,
                             tr("Warning"),
                             tr("Unable to open cache file dir: %1").
                             arg(file_location));

}
