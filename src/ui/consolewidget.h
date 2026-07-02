// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ConsoleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConsoleWidget(const QString &serverPath, QWidget *parent = nullptr);

    void appendLog(const QString &line, bool isError);
    void setRunning(bool running);
    void clear();

signals:
    void commandSent(const QString &serverPath, const QString &cmd);

private slots:
    void onSendCommand();

private:
    QPlainTextEdit *m_logView;
    QLineEdit *m_inputLine;
    QPushButton *m_sendButton;
    QString m_serverPath;
    bool m_running;
};

#endif