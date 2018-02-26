#ifndef APP_H
#define APP_H

#include <QMainWindow>
#include <QSplitter>
#include <QBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QListView>

typedef int32_t i32;

#define PANEL_COUNT 2

class AppWindow : public QMainWindow
{
    Q_OBJECT

public:
    QSplitter* splitter;
    QTabWidget* tabWidget[PANEL_COUNT];
    QWidget* tabContent[PANEL_COUNT];
    QBoxLayout* tcLayout[PANEL_COUNT]; // tab content layout
    QListView* fileItemList[PANEL_COUNT];
    QPushButton* testButton;
    class QFileSystemModel* fsModel;
    QModelIndex fileMid;

    AppWindow(QWidget *parent = 0);
    ~AppWindow();

    bool event(QEvent *event) override;

public slots:
    void fileItemClicked(const QModelIndex& mid);
};

#endif // APP_H
