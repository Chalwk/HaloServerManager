#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QTimer>

class Settings;
class ServerProcess;

class ServerManager : public QObject
{
    Q_OBJECT

public:
    explicit ServerManager(Settings *settings, QObject *parent = nullptr);
    ~ServerManager();

    bool launchServer(const QString &serverPath, int port, const QString &serverType);
    void stopServer(const QString &serverPath);
    void restartServer(const QString &serverPath);
    bool isServerRunning(const QString &serverPath) const;
    ServerProcess* getProcess(const QString &serverPath) const;
    QStringList runningServers() const;

    void setAutoRestart(const QString &serverPath, bool enabled, int delaySeconds = 5);
    bool autoRestart(const QString &serverPath) const;

signals:
    void serverStatusChanged();
    void serverLog(const QString &serverPath, const QString &line, bool isError);
    void serverStateChanged(const QString &serverPath, bool running);

private slots:
    void onProcessLog(const QString &line, bool isError);
    void onProcessStateChanged(bool running);
    void onProcessCrashed();

private:
    void removeProcess(const QString &serverPath);

    QMap<QString, ServerProcess*> m_processes;
    Settings *m_settings;
};

#endif