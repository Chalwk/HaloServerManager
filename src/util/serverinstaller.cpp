// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "util/serverinstaller.h"
#include "util/zipunpacker.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QUrl>
#include <QMessageBox>
#include <QApplication>
#include <QDebug>

ServerInstaller::ServerInstaller(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

void ServerInstaller::installServer(const QString &serverType, const QString &destinationPath)
{
    m_serverType = serverType;
    m_destPath = destinationPath;

    QString serverFolder = QDir(destinationPath).absoluteFilePath(serverType);
    if (QDir(serverFolder).exists())
    {
        if (!confirmOverwrite(destinationPath, serverType))
        {
            emit installationFinished(false, "Installation cancelled by user.");
            return;
        }
    }

    QString url;
    if (serverType == "SAPP_CE")
        url = "https://github.com/Chalwk/SPCLib/releases/download/sapp-server-templates/SAPP_CE.zip";
    else if (serverType == "SAPP_PC")
        url = "https://github.com/Chalwk/SPCLib/releases/download/sapp-server-templates/SAPP_PC.zip";
    else
    {
        emit installationFinished(false, "Unknown server type.");
        return;
    }

    m_tempFile.setFileName(QDir::temp().absoluteFilePath("halo_server_download.zip"));
    if (m_tempFile.open(QIODevice::WriteOnly))
    {
        QNetworkRequest request{QUrl(url)};
        QNetworkReply *reply = m_network->get(request);
        connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total)
                {
            if (total > 0) {
                int percent = static_cast<int>((received * 100) / total);
                emit downloadProgress(percent);
            } });
        connect(reply, &QNetworkReply::finished, this, [this, reply]()
                { onDownloadFinished(reply); });
        connect(reply, &QNetworkReply::readyRead, this, [this, reply]()
                {
            if (m_tempFile.isOpen())
                m_tempFile.write(reply->readAll()); });
    }
    else
    {
        emit installationFinished(false, "Cannot create temporary file.");
    }
}

void ServerInstaller::onDownloadFinished(QNetworkReply *reply)
{
    m_tempFile.close();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit installationFinished(false, reply->errorString());
        return;
    }

    extractZip(m_tempFile.fileName(), m_destPath, m_serverType);
    m_tempFile.remove();
}

void ServerInstaller::extractZip(const QString &zipFile, const QString &destDir, const QString &serverType)
{
    QDir dir(destDir);
    if (!dir.exists())
        dir.mkpath(".");

    ZipUnpacker unpacker;
    bool ok = unpacker.extract(zipFile, destDir);
    if (ok)
    {
        QString folderName = serverType;
        QString installedServerPath = QDir(destDir).absoluteFilePath(folderName);
        if (QDir(installedServerPath).exists())
        {
            emit installedPath(QDir::toNativeSeparators(installedServerPath), serverType);
            emit installationFinished(true, QString());
        }
        else
        {
            emit installationFinished(false, "Extraction succeeded but server folder not found.");
        }
    }
    else
    {
        emit installationFinished(false, "Failed to extract the archive.");
    }
}

bool ServerInstaller::confirmOverwrite(const QString &destDir, const QString &serverType)
{
    QString serverFolder = QDir(destDir).absoluteFilePath(serverType);
    qDebug() << "Checking for existing server folder:" << serverFolder;

    if (!QDir(serverFolder).exists())
    {
        qDebug() << "Folder does not exist, proceeding with installation.";
        return true;
    }

    qDebug() << "Folder exists, prompting user for overwrite.";

    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr,
        "Overwrite Existing Server?",
        QString("The server folder '%1' already exists.\nDo you want to overwrite it?")
            .arg(QDir::toNativeSeparators(serverFolder)),
        QMessageBox::Yes | QMessageBox::No);

    return (reply == QMessageBox::Yes);
}