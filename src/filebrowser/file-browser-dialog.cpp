#include "file-browser-dialog.h"

#include <QtGui>
#include <QApplication>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "ui/loading-view.h"
#include "utils/paint-utils.h"
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

} // namespace

FileBrowserDialog::FileBrowserDialog(const ServerRepo& repo, QWidget *parent)
    : QDialog(parent),
      repo_(repo)
{
    path_ = "/";
    forward_history_ = new QStack<QString>();
    backward_history_ = new QStack<QString>();

    const Account& account = seafApplet->accountManager()->currentAccount();
    data_mgr_ = new DataManager(account);
    file_network_mgr_ = new FileNetworkManager(account);
    file_progress_dialog_ = new FileBrowserProgressDialog(this);

    setWindowTitle(tr("File Browser - %1").arg(repo.name));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //don't help
    setWindowModality(Qt::NonModal);

    createToolBar();
    createLoadingFailedView();
    createFileTable();

    QVBoxLayout *vlayout = new QVBoxLayout;
    setLayout(vlayout);

    stack_ = new QStackedWidget;
    stack_->insertWidget(INDEX_LOADING_VIEW, loading_view_);
    stack_->insertWidget(INDEX_TABLE_VIEW, table_view_);
    stack_->insertWidget(INDEX_LOADING_FAILED_VIEW, loading_failed_view_);

    vlayout->addWidget(toolbar_);
    vlayout->addWidget(stack_);

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
    toolbar_ = new QToolBar(this);

    const int w = ::getDPIScaledSize(24);
    toolbar_->setIconSize(QSize(w, w));

    backward_action_ = new QAction(tr("Back"), toolbar_);
    backward_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack));
    backward_action_->setEnabled(false);
    toolbar_->addAction(backward_action_);
    connect(backward_action_, SIGNAL(triggered()), this, SLOT(onBackwardActionClicked()));

    forward_action_ = new QAction(tr("Forward"), toolbar_);
    forward_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
    forward_action_->setEnabled(false);
    connect(forward_action_, SIGNAL(triggered()), this, SLOT(onForwardActionClicked()));
    toolbar_->addAction(forward_action_);

    path_line_edit_ = new QLineEdit;
    path_line_edit_->setReadOnly(true);
    path_line_edit_->setText(tr(kCurrentPath).arg("/"));
    path_line_edit_->setAlignment(Qt::AlignHCenter | Qt::AlignLeft);
    path_line_edit_->setMinimumWidth(400);
    path_line_edit_->setMaxLength(100);
    path_line_edit_->setFrame(false);
    toolbar_->addWidget(path_line_edit_);

    navigate_home_action_ = new QAction(tr("Home"), toolbar_);
    navigate_home_action_->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon));
    connect(navigate_home_action_, SIGNAL(triggered()), this, SLOT(onNavigateHomeActionClicked()));
    toolbar_->addAction(navigate_home_action_);

    refresh_action_ = new QAction(tr("Refresh"), toolbar_);
    refresh_action_->setIcon(QIcon(":/images/refresh.png"));
    connect(refresh_action_, SIGNAL(triggered()), this, SIGNAL(dirChangedForcely()));
    toolbar_->addAction(refresh_action_);
}

void FileBrowserDialog::createFileTable()
{
    loading_view_ = new LoadingView(this);
    table_view_ = new FileTableView(repo_, this);
    table_model_ = new FileTableModel();
    table_view_->setModel(table_model_);
}

void FileBrowserDialog::onDirChangedForcely()
{
    onDirChanged(true);
}

void FileBrowserDialog::onDirChanged(bool forcely)
{
    stack_->setCurrentIndex(INDEX_LOADING_VIEW);
    data_mgr_->getDirents(repo_.id, path_, forcely);
    path_line_edit_->setText(tr(kCurrentPath).arg(path_));
}

void FileBrowserDialog::onGetDirentsSuccess(const QList<SeafDirent>& dirents)
{
    stack_->setCurrentIndex(INDEX_TABLE_VIEW);
    table_model_->setDirents(dirents);
}

void FileBrowserDialog::onGetDirentsFailed(const ApiError& error)
{
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

void FileBrowserDialog::onDirentClicked(const SeafDirent& dirent)
{
    if (dirent.isDir()) {
        onDirClicked(dirent);
    } else {
        onFileClicked(dirent);
    }
}

void FileBrowserDialog::onDirClicked(const SeafDirent& dir)
{
    if (!path_.endsWith("/")) {
        path_ += "/";
    }

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

void FileBrowserDialog::onFileClicked(const SeafDirent& file)
{
    int task_num = file_network_mgr_->createDownloadTask(repo_.id, path_, file.name);
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
