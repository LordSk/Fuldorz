#ifndef APP_H
#define APP_H

#include <QMainWindow>

class AppWindow : public QMainWindow
{
    Q_OBJECT

public:
    AppWindow(QWidget *parent = 0);
    ~AppWindow();
};

#endif // APP_H
