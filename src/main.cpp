#include "app.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet("QSplitter::handle { background-color: #e2e2e2; }");
    AppWindow w;
    w.show();

    return a.exec();
}
