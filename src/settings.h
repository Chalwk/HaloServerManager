#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject *parent = nullptr);

    void load();
    void save();

    int port() const;
    void setPort(int port);

    QJsonArray servers() const;
    void addServer(const QString &path, const QString &type);
    void removeServer(const QString &path);

    bool autoStart(const QString &path) const;
    void setAutoStart(const QString &path, bool enabled);
    bool autoRestart(const QString &path) const;
    void setAutoRestart(const QString &path, bool enabled);
    int restartDelay(const QString &path) const;
    void setRestartDelay(const QString &path, int seconds);

    QString configFilePath() const;

private:
    QJsonObject m_config;
    QString m_configPath;
};

#endif