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
    m_config["port"] = 2302;
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

int Settings::port() const
{
    return m_config.value("port").toInt(2302);
}

void Settings::setPort(int port)
{
    m_config["port"] = port;
}

QJsonArray Settings::servers() const
{
    return m_config.value("servers").toArray();
}

void Settings::addServer(const QString &path, const QString &type)
{
    QJsonArray arr = servers();
    for (int i = 0; i < arr.size(); ++i)
    {
        QJsonObject obj = arr[i].toObject();
        if (obj.value("path").toString() == path)
            return;
    }
    QJsonObject newServer;
    newServer["path"] = path;
    newServer["type"] = type;
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