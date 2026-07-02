#include "serverinstaller.h"
#include "zipunpacker.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QUrl>

ServerInstaller::ServerInstaller(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

void ServerInstaller::installServer(const QString &serverType, const QString &destinationPath)
{
    m_serverType = serverType;
    m_destPath = destinationPath;

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