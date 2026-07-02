// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "util/zipunpacker.h"
#include <QProcess>
#include <QDir>
#include <QDebug>

bool ZipUnpacker::extract(const QString &zipFilePath, const QString &destDir)
{
    QDir dir(destDir);
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
        {
            qWarning() << "Cannot create destination directory:" << destDir;
            return false;
        }
    }

    QProcess process;
    process.setProgram("powershell");
    QStringList args;
    args << "-Command"
         << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                .arg(zipFilePath)
                .arg(destDir);

    process.start("powershell", args);
    if (!process.waitForFinished(60000))
    {
        qWarning() << "PowerShell timed out";
        return false;
    }

    if (process.exitCode() != 0)
    {
        qWarning() << "PowerShell error:" << process.readAllStandardError();
        return false;
    }

    return true;
}