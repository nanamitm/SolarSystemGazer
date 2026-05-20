#include <QApplication>
#include "mainwindow.h"
#include "appicon.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Solar System Gazer");
    app.setOrganizationName("Qt6 Demo");
    app.setWindowIcon(createAppIcon());

    MainWindow w;
    w.show();
    return app.exec();
}
