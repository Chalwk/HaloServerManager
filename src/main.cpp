// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Halo Server Manager");
    app.setOrganizationName("Chalwk");

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}