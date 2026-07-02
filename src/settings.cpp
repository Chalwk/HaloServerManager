// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "settings.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

Settings::Settings(QObject *parent)
    : QObject(parent), m_configPath(configFilePath())
{
}

QString Settings::configFilePath() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(dataDir);
    if (!dir.exists())
        dir.mkpath(".");
    return dir.absoluteFilePath("config.json");
}

void Settings::load()
{
    QFile file(m_configPath);
    if (file.open(QIODevice::ReadOnly))
    {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (doc.isObject())
        {
            m_config = doc.object();
            return;
        }
    }
    m_config = QJsonObject();
    m_config["servers"] = QJsonArray();
}

void Settings::save()
{
    QJsonDocument doc(m_config);
    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        file.write(doc.toJson());
        file.close();
    }
}

QJsonArray Settings::servers() const
{
    return m_config.value("servers").toArray();
}

void Settings::addServer(const QString &path, const QString &type, int port)
{
    QJsonArray arr = servers();
    for (const QJsonValue &val : arr)
    {
        if (val.toObject().value("path").toString() == path)
            return;
    }
    QJsonObject newServer;
    newServer["path"] = path;
    newServer["type"] = type;
    newServer["port"] = port;
    newServer["autoStart"] = false;
    newServer["autoRestart"] = false;
    newServer["restartDelay"] = 5;
    arr.append(newServer);
    m_config["servers"] = arr;
}

void Settings::removeServer(const QString &path)
{
    QJsonArray arr = servers();
    for (int i = 0; i < arr.size(); ++i)
    {
        if (arr[i].toObject().value("path").toString() == path)
        {
            arr.removeAt(i);
            break;
        }
    }
    m_config["servers"] = arr;
}

int Settings::serverPort(const QString &path) const
{
    QJsonArray arr = servers();
    for (const QJsonValue &val : arr)
    {
        QJsonObject obj = val.toObject();
        if (obj.value("path").toString() == path)
            return obj.value("port").toInt(2302);
    }
    return 2302;
}

void Settings::setServerPort(const QString &path, int port)
{
    QJsonArray arr = servers();
    for (int i = 0; i < arr.size(); ++i)
    {
        QJsonObject obj = arr[i].toObject();
        if (obj.value("path").toString() == path)
        {
            obj["port"] = port;
            arr[i] = obj;
            m_config["servers"] = arr;
            break;
        }
    }
}

bool Settings::autoStart(const QString &path) const
{
    QJsonArray arr = servers();
    for (const QJsonValue &val : arr)
    {
        QJsonObject obj = val.toObject();
        if (obj.value("path").toString() == path)
            return obj.value("autoStart").toBool(false);
    }
    return false;
}

void Settings::setAutoStart(const QString &path, bool enabled)
{
    QJsonArray arr = servers();
    for (int i = 0; i < arr.size(); ++i)
    {
        QJsonObject obj = arr[i].toObject();
        if (obj.value("path").toString() == path)
        {
            obj["autoStart"] = enabled;
            arr[i] = obj;
            m_config["servers"] = arr;
            break;
        }
    }
}

bool Settings::autoRestart(const QString &path) const
{
    QJsonArray arr = servers();
    for (const QJsonValue &val : arr)
    {
        QJsonObject obj = val.toObject();
        if (obj.value("path").toString() == path)
            return obj.value("autoRestart").toBool(false);
    }
    return false;
}

void Settings::setAutoRestart(const QString &path, bool enabled)
{
    QJsonArray arr = servers();
    for (int i = 0; i < arr.size(); ++i)
    {
        QJsonObject obj = arr[i].toObject();
        if (obj.value("path").toString() == path)
        {
            obj["autoRestart"] = enabled;
            arr[i] = obj;
            m_config["servers"] = arr;
            break;
        }
    }
}

int Settings::restartDelay(const QString &path) const
{
    QJsonArray arr = servers();
    for (const QJsonValue &val : arr)
    {
        QJsonObject obj = val.toObject();
        if (obj.value("path").toString() == path)
            return obj.value("restartDelay").toInt(5);
    }
    return 5;
}

void Settings::setRestartDelay(const QString &path, int seconds)
{
    QJsonArray arr = servers();
    for (int i = 0; i < arr.size(); ++i)
    {
        QJsonObject obj = arr[i].toObject();
        if (obj.value("path").toString() == path)
        {
            obj["restartDelay"] = seconds;
            arr[i] = obj;
            m_config["servers"] = arr;
            break;
        }
    }
}