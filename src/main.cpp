// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include <QApplication>
#include "ui/mainwindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Halo Server Manager");
    app.setOrganizationName("Chalwk");
    app.setApplicationVersion(PROJECT_VERSION);

    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}