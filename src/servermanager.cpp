// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "servermanager.h"
#include "serverprocess.h"
#include "settings.h"
#include <QDebug>

ServerManager::ServerManager(Settings *settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

ServerManager::~ServerManager()
{
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it)
    {
        it.value()->stop();
        delete it.value();
    }
    m_processes.clear();
}

bool ServerManager::launchServer(const QString &serverPath, int port, const QString &serverType)
{
    if (m_processes.contains(serverPath))
    {
        if (isServerRunning(serverPath))
        {
            qWarning() << "Server already running at" << serverPath;
            return false;
        }
        else
        {
            removeProcess(serverPath);
        }
    }

    ServerProcess *proc = new ServerProcess(serverPath, serverType, port, this);
    connect(proc, &ServerProcess::logLine, this, &ServerManager::onProcessLog);
    connect(proc, &ServerProcess::stateChanged, this, &ServerManager::onProcessStateChanged);
    connect(proc, &ServerProcess::processCrashed, this, &ServerManager::onProcessCrashed);

    if (proc->start())
    {
        m_processes.insert(serverPath, proc);
        emit serverStatusChanged();
        return true;
    }
    else
    {
        delete proc;
        return false;
    }
}

void ServerManager::stopServer(const QString &serverPath)
{
    ServerProcess *proc = m_processes.value(serverPath, nullptr);
    if (proc)
    {
        proc->stop();
    }
}

void ServerManager::restartServer(const QString &serverPath)
{
    ServerProcess *proc = m_processes.value(serverPath, nullptr);
    if (proc)
    {
        proc->restart();
    }
}

bool ServerManager::isServerRunning(const QString &serverPath) const
{
    ServerProcess *proc = m_processes.value(serverPath, nullptr);
    return proc && proc->isRunning();
}

ServerProcess *ServerManager::getProcess(const QString &serverPath) const
{
    return m_processes.value(serverPath, nullptr);
}

QStringList ServerManager::runningServers() const
{
    QStringList list;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it)
    {
        if (it.value()->isRunning())
            list << it.key();
    }
    return list;
}

void ServerManager::setAutoRestart(const QString &serverPath, bool enabled, int delaySeconds)
{
    ServerProcess *proc = m_processes.value(serverPath, nullptr);
    if (proc)
        proc->setAutoRestart(enabled, delaySeconds);
}

bool ServerManager::autoRestart(const QString &serverPath) const
{
    ServerProcess *proc = m_processes.value(serverPath, nullptr);
    return proc ? proc->autoRestart() : false;
}

void ServerManager::onProcessLog(const QString &line, bool isError)
{
    ServerProcess *proc = qobject_cast<ServerProcess *>(sender());
    if (!proc)
        return;
    QString path = proc->serverPath();
    emit serverLog(path, line, isError);
}

void ServerManager::onProcessStateChanged(bool running)
{
    ServerProcess *proc = qobject_cast<ServerProcess *>(sender());
    if (!proc)
        return;
    QString path = proc->serverPath();

    if (!running)
    {
        if (!proc->isRestarting() && !proc->autoRestart())
        {
            QTimer::singleShot(100, this, [this, path]()
                               { removeProcess(path); });
        }
    }

    emit serverStateChanged(path, running);
    emit serverStatusChanged();
}

void ServerManager::onProcessCrashed()
{
    // Just forward, stateChanged will handle removal
}

void ServerManager::removeProcess(const QString &serverPath)
{
    ServerProcess *proc = m_processes.take(serverPath);
    if (proc)
    {
        proc->deleteLater();
    }
}