#include "consolewidget.h"
#include <QKeyEvent>
#include <QRegularExpression>

static QString stripAnsi(const QString &text)
{
    static QRegularExpression re("\x1b\\[[0-9;?><=!]*[a-zA-Z]");
    QString cleaned = text;
    cleaned.remove(re);
    cleaned.remove(QChar(0x1B));
    return cleaned;
}

ConsoleWidget::ConsoleWidget(const QString &serverPath, QWidget *parent)
    : QWidget(parent), m_serverPath(serverPath), m_running(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Courier New", 9));
    m_logView->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(m_logView);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_inputLine = new QLineEdit(this);
    m_inputLine->setEnabled(false);
    m_inputLine->setPlaceholderText("Enter command (enabled when server is running)");
    connect(m_inputLine, &QLineEdit::returnPressed, this, &ConsoleWidget::onSendCommand);

    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setEnabled(false);
    connect(m_sendButton, &QPushButton::clicked, this, &ConsoleWidget::onSendCommand);

    inputLayout->addWidget(m_inputLine);
    inputLayout->addWidget(m_sendButton);
    layout->addLayout(inputLayout);

    setRunning(false);
}

void ConsoleWidget::appendLog(const QString &line, bool isError)
{
    QString cleaned = stripAnsi(line);
    QString prefix = isError ? "[ERROR] " : "";
    m_logView->appendPlainText(prefix + cleaned);
    m_logView->moveCursor(QTextCursor::End);
}

void ConsoleWidget::setRunning(bool running)
{
    m_running = running;
    m_inputLine->setEnabled(running);
    m_sendButton->setEnabled(running);
    if (!running)
    {
        m_inputLine->setPlaceholderText("Server is not running");
    }
    else
    {
        m_inputLine->setPlaceholderText("Type command and press Enter");
        m_inputLine->setFocus();
    }
}

void ConsoleWidget::clear()
{
    m_logView->clear();
}

void ConsoleWidget::onSendCommand()
{
    QString cmd = m_inputLine->text().trimmed();
    if (cmd.isEmpty() || !m_running)
        return;
    emit commandSent(m_serverPath, cmd);
    m_inputLine->clear();
    appendLog("> " + cmd, false);
}