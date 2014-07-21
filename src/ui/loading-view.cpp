#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "loading-view.h"

LoadingView::LoadingView(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("LoadingView");

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    gif_ = new QMovie(":/images/loading.gif");
    QLabel *label = new QLabel;
    label->setMovie(gif_);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
}

void LoadingView::showEvent(QShowEvent *event)
{
    gif_->start();
    QWidget::showEvent(event);
}

void LoadingView::hideEvent(QHideEvent *event)
{
    gif_->stop();
    QWidget::hideEvent(event);
}
