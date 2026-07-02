// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#ifndef SERVERPROCESS_H
#define SERVERPROCESS_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#include <QWinEventNotifier>

#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif

typedef HRESULT(WINAPI *CreatePseudoConsole_t)(COORD size, HANDLE hInput, HANDLE hOutput, DWORD dwFlags, HPCON *phPC);
typedef void(WINAPI *ClosePseudoConsole_t)(HPCON hPC);

#endif

class ServerProcess : public QObject
{
    Q_OBJECT
public:
    explicit ServerProcess(const QString &serverPath, const QString &serverType, int port, QObject *parent = nullptr);
    ~ServerProcess();

    bool start();
    void stop();
    void restart();
    bool isRunning() const;
    void sendCommand(const QString &cmd);

    QString serverPath() const { return m_serverPath; }
    QString serverType() const { return m_serverType; }
    int port() const { return m_port; }
    qint64 uptime() const;

    void setAutoRestart(bool enabled, int delaySeconds = 5);
    bool autoRestart() const { return m_autoRestart; }
    bool isRestarting() const { return m_restarting; }

signals:
    void logLine(const QString &line, bool isError);
    void stateChanged(bool running);
    void processCrashed();

private slots:
    void onProcessExited();
    void onPipeReady();
    void onAutoRestart();

private:
    void CleanupPseudoConsoleAndPipes();
    bool initConPtyFunctions();

    QString m_serverPath;
    QString m_serverType;
    int m_port;
    bool m_autoRestart;
    int m_restartDelay;
    QTimer *m_restartTimer;
    QDateTime m_startTime;
    bool m_restarting;

    QByteArray m_stdoutBuffer;

#ifdef Q_OS_WIN
    HANDLE m_hProcess;
    DWORD m_pid;
    HPCON m_console;
    HANDLE m_outputPipe;
    HANDLE m_inputPipe;
    QWinEventNotifier *m_outputNotifier;
    QWinEventNotifier *m_processNotifier;

    CreatePseudoConsole_t pCreatePseudoConsole;
    ClosePseudoConsole_t pClosePseudoConsole;
#endif
};

#endif