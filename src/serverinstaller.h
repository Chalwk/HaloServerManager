// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#ifndef SERVERINSTALLER_H
#define SERVERINSTALLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QString>

class ServerInstaller : public QObject
{
    Q_OBJECT

public:
    explicit ServerInstaller(QObject *parent = nullptr);

    void installServer(const QString &serverType, const QString &destinationPath);

signals:
    void downloadProgress(int percent);
    void installationFinished(bool success, const QString &message);
    void installedPath(const QString &path, const QString &serverType);

private slots:
    void onDownloadFinished(QNetworkReply *reply);

private:
    void extractZip(const QString &zipFile, const QString &destDir, const QString &serverType);
    bool confirmOverwrite(const QString &destDir, const QString &serverType);

    QNetworkAccessManager *m_network;
    QString m_serverType;
    QString m_destPath;
    QFile m_tempFile;
};

#endif