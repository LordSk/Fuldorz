#include "app.h"
#include <QKeyEvent>
#include <QDir>
#include <QFileSystemModel>
#include <QDebug>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

AppWindow::AppWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    splitter = new QSplitter(this);

    for(i32 p = 0; p < PANEL_COUNT; ++p) {
        tabWidget[p] = new QTabWidget(splitter);
    }

    for(i32 p = 0; p < PANEL_COUNT; ++p) {
        tabContent[p] = new QWidget();
    }

    for(i32 p = 0; p < PANEL_COUNT; ++p) {
        tcLayout[p] = new QBoxLayout(QBoxLayout::TopToBottom, tabContent[p]);
    }

    for(i32 p = 0; p < PANEL_COUNT; ++p) {
        fileItemList[p] = new QListView();
    }

    for(i32 p = 0; p < PANEL_COUNT; ++p) {
        tabContent[p]->setLayout(tcLayout[p]);
    }

    fsModel = new QFileSystemModel();
    //fsModel->setRootPath(QDir::root().absolutePath());
    fsModel->setRootPath("C:/");
    fileItemList[0]->setModel(fsModel);

    connect(fileItemList[0], SIGNAL(clicked(QModelIndex)), this, SLOT(fileItemClicked(QModelIndex)));

    for(i32 p = 0; p < PANEL_COUNT; ++p) {
        tabWidget[p]->addTab(tabContent[p], "Tab0");
        tcLayout[p]->addWidget(fileItemList[p]);
        splitter->addWidget(tabWidget[p]);
    }

    splitter->setSizes({WINDOW_WIDTH/PANEL_COUNT, WINDOW_WIDTH/PANEL_COUNT});

    setCentralWidget(splitter);
}

AppWindow::~AppWindow()
{

}

bool AppWindow::event(QEvent *event)
{
    if(event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);

        // close on escape
        if(ke->key() == Qt::Key_Escape) {
            close();
            return true;
        }
        if(ke->key() == Qt::Key_Backspace) {
            fileMid = fsModel->parent(fileMid);
            fileItemList[0]->setRootIndex(fileMid);
            qDebug() << "BACK BUTTON";
            return true;
        }
    }

    if(event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        if(me->button() == Qt::BackButton) {

            return true;
        }
    }

    return QMainWindow::event(event);
}

void AppWindow::fileItemClicked(const QModelIndex &mid)
{
    if(fsModel->isDir(mid)) {
        fileMid = mid;
        fileItemList[0]->setRootIndex(mid);
        const QString& path = fsModel->filePath(mid);
        qDebug() << path;
    }
}
