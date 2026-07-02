// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "config/settings.h"
#include "config/configeditor.h"
#include "core/servermanager.h"
#include "core/serverprocess.h"
#include "ui/mainwindow.h"
#include "ui/consolewidget.h"
#include "util/serverinstaller.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QJsonArray>
#include <QSplitter>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QCheckBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QStyleFactory>
#include <QApplication>
#include <QTabWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QSpinBox>
#include <QDesktopServices>
#include <QThread>
#include <QUdpSocket>
#include <QDebug>
#include <QHostAddress>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_installer(nullptr), m_manager(nullptr), m_settings(new Settings(this))
{
    setWindowTitle("Halo Server Manager");
    resize(1100, 650);
    setupUi();
    loadConfig();
    createTrayIcon();

    QTimer *queryTimer = new QTimer(this);
    connect(queryTimer, &QTimer::timeout, this, &MainWindow::updateServerStatus);
    queryTimer->start(2000);

    updateServerStatus();
    updateInstalledStatus();
}

MainWindow::~MainWindow()
{
    for (auto it = m_pendingQueries.begin(); it != m_pendingQueries.end(); ++it)
    {
        it.value()->deleteLater();
    }
    m_pendingQueries.clear();
    m_settings->save();
}

void MainWindow::setupUi()
{
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);

    QWidget *installTab = new QWidget();
    QVBoxLayout *installLayout = new QVBoxLayout(installTab);

    QGroupBox *typeGroup = new QGroupBox("Server Type");
    QHBoxLayout *typeLayout = new QHBoxLayout(typeGroup);
    m_serverTypeCombo = new QComboBox();
    m_serverTypeCombo->addItems({"SAPP_CE", "SAPP_PC"});
    typeLayout->addWidget(new QLabel("Select:"));
    typeLayout->addWidget(m_serverTypeCombo);

    m_installStatusLabel = new QLabel(this);
    m_installStatusLabel->setIndent(10);
    typeLayout->addWidget(m_installStatusLabel);
    typeLayout->addStretch();
    installLayout->addWidget(typeGroup);

    QGroupBox *pathGroup = new QGroupBox("Installation Directory");
    QHBoxLayout *pathLayout = new QHBoxLayout(pathGroup);
    m_installPathEdit = new QLineEdit();
    m_installPathEdit->setReadOnly(true);
    m_installPathEdit->setText(QDir::toNativeSeparators("C:/Halo Servers"));
    m_browseButton = new QPushButton("Browse...");
    connect(m_browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseInstallPath);
    pathLayout->addWidget(m_installPathEdit, 1);
    pathLayout->addWidget(m_browseButton);
    installLayout->addWidget(pathGroup);

    m_installButton = new QPushButton("Download and Install");
    m_installButton->setEnabled(false);
    connect(m_installButton, &QPushButton::clicked, this, &MainWindow::onInstallClicked);
    installLayout->addWidget(m_installButton);

    m_downloadProgress = new QProgressBar();
    m_downloadProgress->setVisible(false);
    installLayout->addWidget(m_downloadProgress);

    m_installProgressLabel = new QLabel();
    m_installProgressLabel->setWordWrap(true);
    installLayout->addWidget(m_installProgressLabel);

    installLayout->addStretch();
    m_tabWidget->addTab(installTab, "Install");

    connect(m_serverTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateInstalledStatus);
    connect(m_installPathEdit, &QLineEdit::textChanged,
            this, &MainWindow::updateInstalledStatus);

    QWidget *serversTab = new QWidget();
    QHBoxLayout *serversLayout = new QHBoxLayout(serversTab);

    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *listLabel = new QLabel("Servers");
    listLabel->setStyleSheet("font-weight: bold;");
    leftLayout->addWidget(listLabel);

    m_serverList = new QListWidget();
    m_serverList->setMinimumWidth(300);
    connect(m_serverList, &QListWidget::currentRowChanged, this, &MainWindow::onServerSelectionChanged);
    leftLayout->addWidget(m_serverList);

    m_queryInfoGroup = new QGroupBox("Server Information");
    QFormLayout *infoLayout = new QFormLayout(m_queryInfoGroup);
    m_hostnameLabel = new QLabel("N/A");
    m_mapLabel = new QLabel("N/A");
    m_gametypeLabel = new QLabel("N/A");
    m_variantLabel = new QLabel("N/A");
    m_playersLabel = new QLabel("N/A");
    m_teamplayLabel = new QLabel("N/A");
    m_fraglimitLabel = new QLabel("N/A");
    infoLayout->addRow("Hostname:", m_hostnameLabel);
    infoLayout->addRow("Map:", m_mapLabel);
    infoLayout->addRow("Game Type:", m_gametypeLabel);
    infoLayout->addRow("Mode:", m_variantLabel);
    infoLayout->addRow("Players:", m_playersLabel);
    infoLayout->addRow("Teamplay:", m_teamplayLabel);
    infoLayout->addRow("Score Limit:", m_fraglimitLabel);
    leftLayout->addWidget(m_queryInfoGroup);
    leftLayout->addStretch();

    m_contentStack = new QStackedWidget();
    QLabel *noSelectionLabel = new QLabel("Select a server from the left");
    noSelectionLabel->setAlignment(Qt::AlignCenter);
    m_contentStack->addWidget(noSelectionLabel);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(m_contentStack);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({80, 600});
    serversLayout->addWidget(splitter);
    m_tabWidget->addTab(serversTab, "Servers");

    m_toolBar = new QToolBar(this);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addToolBar(Qt::TopToolBarArea, m_toolBar);

    m_aboutAction = new QAction("About", this);
    m_aboutAction->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    m_toolBar->addAction(m_aboutAction);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setTextFormat(Qt::RichText);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusBar()->addPermanentWidget(m_statusLabel, 1);
}

void MainWindow::loadConfig()
{
    m_settings->load();

    if (!m_manager)
    {
        m_manager = new ServerManager(m_settings, this);
        connect(m_manager, &ServerManager::serverStatusChanged, this, &MainWindow::updateServerStatus);
        connect(m_manager, &ServerManager::serverLog, this, &MainWindow::onServerLog);
        connect(m_manager, &ServerManager::serverStateChanged, this, &MainWindow::onServerStateChanged);
    }

    refreshServerList();
    m_installButton->setEnabled(!m_installPathEdit->text().isEmpty());

    QJsonArray servers = m_settings->servers();
    for (const QJsonValue &val : servers)
    {
        QJsonObject obj = val.toObject();
        if (obj.value("autoStart").toBool(false))
        {
            QString path = obj["path"].toString();
            QString type = obj["type"].toString();
            int port = obj["port"].toInt(2302);
            m_manager->launchServer(path, port, type);
        }
    }
}

void MainWindow::onBrowseInstallPath()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Installation Folder",
                                                    QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    if (!dir.isEmpty())
    {
        m_installPathEdit->setText(QDir::toNativeSeparators(dir));
        m_installButton->setEnabled(true);
        updateInstalledStatus();
    }
}

void MainWindow::onInstallClicked()
{
    const QString type = m_serverTypeCombo->currentText();
    const QString destPath = m_installPathEdit->text();
    if (destPath.isEmpty())
        return;

    m_installButton->setEnabled(false);
    m_downloadProgress->setVisible(true);
    m_downloadProgress->setValue(0);
    m_installProgressLabel->setText("Downloading...");

    if (!m_installer)
    {
        m_installer = new ServerInstaller(this);
        connect(m_installer, &ServerInstaller::downloadProgress, m_downloadProgress, &QProgressBar::setValue);
        connect(m_installer, &ServerInstaller::installationFinished, this, [this](bool success, const QString &message)
                {
                    m_downloadProgress->setVisible(false);
                    m_installButton->setEnabled(!m_installPathEdit->text().isEmpty());
                    if (success) {
                        m_installProgressLabel->setText("Installation completed successfully.");
                        updateInstalledStatus();
                    } else {
                        m_installProgressLabel->setText("Installation failed: " + message);
                    } });
        connect(m_installer, &ServerInstaller::installedPath, this, [this](const QString &path, const QString &type)
                {
                    m_settings->addServer(path, type, 2302);
                    m_settings->save();
                    refreshServerList();
                    m_tabWidget->setCurrentIndex(1);
                    for (int i = 0; i < m_serverList->count(); ++i) {
                        if (m_serverList->item(i)->data(Qt::UserRole).toString() == path) {
                            m_serverList->setCurrentRow(i);
                            break;
                        }
                    } });
    }
    m_installer->installServer(type, destPath);
}

void MainWindow::refreshServerList()
{
    m_serverList->clear();
    const QJsonArray servers = m_settings->servers();
    for (const QJsonValue &val : servers)
    {
        QJsonObject obj = val.toObject();
        QString path = obj["path"].toString();
        QString type = obj["type"].toString();
        QListWidgetItem *item = new QListWidgetItem(type);
        item->setData(Qt::UserRole, path);
        item->setData(Qt::UserRole + 1, type);
        m_serverList->addItem(item);
    }
    updateServerListStatus();

    if (m_serverList->count() > 0)
        m_serverList->setCurrentRow(0);
    onServerSelectionChanged();
}

void MainWindow::updateServerListStatus()
{
    for (int i = 0; i < m_serverList->count(); ++i)
    {
        QListWidgetItem *item = m_serverList->item(i);
        QString path = item->data(Qt::UserRole).toString();
        QString type = item->data(Qt::UserRole + 1).toString();
        bool running = m_manager && m_manager->isServerRunning(path);

        QString statusText = QString("%1 [%2] %3")
                                 .arg(type)
                                 .arg(running ? "🟢" : "🔴")
                                 .arg(running ? "RUNNING" : "STOPPED");
        item->setText(statusText);
        item->setForeground(running ? Qt::darkGreen : Qt::red);
    }
}

QWidget *MainWindow::getServerDetailWidget(const QString &serverPath)
{
    if (m_serverDetailWidgets.contains(serverPath))
        return m_serverDetailWidgets[serverPath];

    QWidget *container = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    QPushButton *startBtn = new QPushButton("Start");
    startBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    QPushButton *stopBtn = new QPushButton("Stop");
    stopBtn->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    QPushButton *restartBtn = new QPushButton("Restart");
    restartBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));

    m_startButtons[serverPath] = startBtn;
    m_stopButtons[serverPath] = stopBtn;
    m_restartButtons[serverPath] = restartBtn;

    connect(startBtn, &QPushButton::clicked, this, [this, serverPath]()
            {
                QJsonArray servers = m_settings->servers();
                QString type;
                int port = 2302;
                for (const QJsonValue &val : servers) {
                    QJsonObject obj = val.toObject();
                    if (obj["path"].toString() == serverPath) {
                        type = obj["type"].toString();
                        port = obj["port"].toInt(2302);
                        break;
                    }
                }
                if (type.isEmpty())
                    return;
                ConsoleWidget *console = getConsoleForServer(serverPath);
                if (console)
                    console->clear();
                if (m_manager->launchServer(serverPath, port, type)) {
                    statusBar()->showMessage("Server launched: " + serverPath, 3000);
                    updateServerStatus();
                    showConsoleForServer(serverPath);
                } else {
                    statusBar()->showMessage("Failed to launch server", 3000);
                } });

    connect(stopBtn, &QPushButton::clicked, this, [this, serverPath]()
            {
                m_manager->stopServer(serverPath);
                statusBar()->showMessage("Server stopped: " + serverPath, 3000);
                updateServerStatus(); });

    connect(restartBtn, &QPushButton::clicked, this, [this, serverPath]()
            {
                m_manager->restartServer(serverPath);
                statusBar()->showMessage("Restarting server: " + serverPath, 3000);
                updateServerStatus(); });

    QTabWidget *tabWidget = new QTabWidget;

    QWidget *cornerWidget = new QWidget(tabWidget);
    QHBoxLayout *cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 0, 0);
    cornerLayout->setSpacing(6);
    cornerLayout->addWidget(startBtn);
    cornerLayout->addWidget(stopBtn);
    cornerLayout->addWidget(restartBtn);
    tabWidget->setCornerWidget(cornerWidget, Qt::TopRightCorner);

    ConsoleWidget *console = new ConsoleWidget(serverPath, this);
    connect(console, &ConsoleWidget::commandSent, this, [this](const QString &path, const QString &cmd)
            {
                if (m_manager) {
                    ServerProcess *proc = m_manager->getProcess(path);
                    if (proc)
                        proc->sendCommand(cmd);
                } });
    m_consoles[serverPath] = console;
    tabWidget->addTab(console, "Console");

    QWidget *settingsPanel = new QWidget;
    QFormLayout *form = new QFormLayout(settingsPanel);

    QSpinBox *portSpin = new QSpinBox;
    portSpin->setRange(1, 65535);
    portSpin->setValue(m_settings->serverPort(serverPath));
    form->addRow("Server Port:", portSpin);

    QCheckBox *autoRestartCheck = new QCheckBox;
    autoRestartCheck->setChecked(m_settings->autoRestart(serverPath));
    form->addRow("Auto-Restart:", autoRestartCheck);

    QSpinBox *delaySpin = new QSpinBox;
    delaySpin->setRange(1, 300);
    delaySpin->setValue(m_settings->restartDelay(serverPath));
    form->addRow("Restart Delay (s):", delaySpin);

    QPushButton *saveBtn = new QPushButton("Save Settings");
    form->addRow(saveBtn);

    QPushButton *editFilesBtn = new QPushButton("Edit Server Files");
    connect(editFilesBtn, &QPushButton::clicked, this, [this, serverPath]()
            {
                ConfigEditor editor(serverPath, this);
                editor.exec(); });
    form->addRow(editFilesBtn);

    QPushButton *openDirBtn = new QPushButton("Open Server Directory");
    connect(openDirBtn, &QPushButton::clicked, this, [this, serverPath]()
            { QDesktopServices::openUrl(QUrl::fromLocalFile(serverPath)); });
    form->addRow(openDirBtn);

    QPushButton *uninstallBtn = new QPushButton("Uninstall Server");
    uninstallBtn->setStyleSheet("QPushButton { color: red; }");
    connect(uninstallBtn, &QPushButton::clicked, this, &MainWindow::onUninstallServer);
    form->addRow(uninstallBtn);

    connect(saveBtn, &QPushButton::clicked, this, [this, serverPath, portSpin, autoRestartCheck, delaySpin]()
            {
                m_settings->setServerPort(serverPath, portSpin->value());
                m_settings->setAutoRestart(serverPath, autoRestartCheck->isChecked());
                m_settings->setRestartDelay(serverPath, delaySpin->value());
                m_settings->save();
                if (m_manager) {
                    m_manager->setAutoRestart(serverPath, autoRestartCheck->isChecked(), delaySpin->value());
                }
                statusBar()->showMessage("Settings saved for " + serverPath, 3000); });

    tabWidget->addTab(settingsPanel, "Settings");

    layout->addWidget(tabWidget);
    container->setLayout(layout);

    m_serverDetailWidgets[serverPath] = container;
    return container;
}

void MainWindow::showConsoleForServer(const QString &serverPath)
{
    QWidget *detailWidget = getServerDetailWidget(serverPath);
    if (detailWidget)
    {
        int index = m_contentStack->indexOf(detailWidget);
        if (index == -1)
            index = m_contentStack->addWidget(detailWidget);
        m_contentStack->setCurrentIndex(index);
    }
}

ConsoleWidget *MainWindow::getConsoleForServer(const QString &serverPath)
{
    if (m_consoles.contains(serverPath))
        return m_consoles[serverPath];
    ConsoleWidget *console = new ConsoleWidget(serverPath, this);
    connect(console, &ConsoleWidget::commandSent, this, [this](const QString &path, const QString &cmd)
            {
                if (m_manager) {
                    ServerProcess *proc = m_manager->getProcess(path);
                    if (proc)
                        proc->sendCommand(cmd);
                } });
    m_consoles[serverPath] = console;
    return console;
}

void MainWindow::onServerSelectionChanged()
{
    int idx = m_serverList->currentRow();
    if (idx >= 0)
    {
        QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
        showConsoleForServer(path);
        updateQueryInfoDisplay(path);
    }
    else
    {
        m_contentStack->setCurrentIndex(0);
        m_hostnameLabel->setText("N/A");
        m_mapLabel->setText("N/A");
        m_gametypeLabel->setText("N/A");
        m_variantLabel->setText("N/A");
        m_playersLabel->setText("N/A");
        m_teamplayLabel->setText("N/A");
        m_fraglimitLabel->setText("N/A");
    }
    updateServerStatus();
}

void MainWindow::updateServerStatus()
{
    updateServerListStatus();

    for (const QString &path : m_startButtons.keys())
    {
        bool running = m_manager && m_manager->isServerRunning(path);
        m_startButtons[path]->setEnabled(!running);
        m_stopButtons[path]->setEnabled(running);
        m_restartButtons[path]->setEnabled(true);
    }

    QJsonArray servers = m_settings->servers();
    for (const QJsonValue &val : servers)
    {
        QJsonObject obj = val.toObject();
        QString path = obj["path"].toString();
        bool running = m_manager && m_manager->isServerRunning(path);
        if (running)
        {
            int port = obj["port"].toInt(2302);
            if (!m_pendingQueries.contains(path))
                sendServerQuery(path, port);
        }
        else
        {
            if (m_pendingQueries.contains(path))
            {
                m_pendingQueries[path]->deleteLater();
                m_pendingQueries.remove(path);
            }
        }
    }

    int idx = m_serverList->currentRow();
    if (idx < 0)
    {
        m_statusLabel->setText("<b>No server selected</b>");
        return;
    }
    QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
    bool running = m_manager && m_manager->isServerRunning(path);

    ConsoleWidget *console = m_consoles.value(path, nullptr);
    if (console)
        console->setRunning(running);

    if (running)
    {
        ServerProcess *proc = m_manager->getProcess(path);
        qint64 uptime = proc ? proc->uptime() : 0;
        QString html = QString(
                           "<b style='color: green;'>%1</b> "
                           "<span style='color: gray;'>|</span> "
                           "<b>Running</b> for <b>%2</b> seconds")
                           .arg(QDir::toNativeSeparators(path))
                           .arg(uptime);
        m_statusLabel->setText(html);
    }
    else
    {
        QString html = QString(
                           "<b style='color: red;'>%1</b> "
                           "<span style='color: gray;'>|</span> "
                           "<b>Stopped</b>")
                           .arg(QDir::toNativeSeparators(path));
        m_statusLabel->setText(html);
    }
}

void MainWindow::sendServerQuery(const QString &serverPath, int port)
{
    QUdpSocket *socket = new QUdpSocket(this);
    m_pendingQueries[serverPath] = socket;

    connect(socket, &QUdpSocket::readyRead, this, [this, serverPath, socket]()
            {
        while (socket->hasPendingDatagrams()) {
            QByteArray data;
            data.resize(socket->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;
            socket->readDatagram(data.data(), data.size(), &sender, &senderPort);
            qDebug() << "Received datagram from" << sender.toString() << senderPort << "size:" << data.size();
            qDebug() << "Raw data:" << data;
            parseQueryReply(serverPath, data);
        }
        socket->deleteLater();
        m_pendingQueries.remove(serverPath); });

    QTimer::singleShot(2000, this, [this, serverPath, socket]()
                       {
        if (m_pendingQueries.contains(serverPath) && m_pendingQueries[serverPath] == socket) {
            qDebug() << "Query timeout for" << serverPath;
            socket->deleteLater();
            m_pendingQueries.remove(serverPath);
        } });

    QHostAddress address("127.0.0.1");
    QByteArray query = "\\query";
    qDebug() << "Sending query to" << address.toString() << "port" << port << "for server" << serverPath;
    socket->writeDatagram(query, address, port);
}

void MainWindow::parseQueryReply(const QString &serverPath, const QByteArray &reply)
{
    QString replyStr = QString::fromUtf8(reply);
    qDebug() << "Parsing reply for" << serverPath << ":" << replyStr;

    QStringList parts = replyStr.split('\\', Qt::SkipEmptyParts);
    qDebug() << "Parts:" << parts;

    ServerQueryInfo info;
    info.valid = true;

    int players = -1;
    int maxPlayers = -1;

    for (int i = 0; i < parts.size() - 1; i += 2)
    {
        QString key = parts[i].toLower();
        QString value = parts[i + 1];

        if (key == "hostname" || key == "sv_hostname")
        {
            info.hostname = value;
        }
        else if (key == "mapname" || key == "sv_mapname")
        {
            info.mapname = value;
        }
        else if (key == "gametype" || key == "sv_gametype")
        {
            info.gametype = value;
        }
        else if (key == "gamevariant" || key == "sv_gamevariant" || key == "sv_variant")
        {
            info.gamevariant = value;
        }
        else if (key == "teamplay" || key == "sv_teamplay")
        {
            info.teamplay = (value.toInt() == 1);
        }
        else if (key == "fraglimit" || key == "sv_fraglimit")
        {
            info.fraglimit = value.toInt();
        }
        else if (key == "maxplayers" || key == "sv_maxplayers")
        {
            maxPlayers = value.toInt();
            info.maxplayers = maxPlayers;
        }
        else if (key == "numplayers" || key == "players" || key == "sv_players")
        {
            bool ok;
            int num = value.toInt(&ok);
            if (ok)
            {
                players = num;
            }
            else
            {
                players = value.split(';', Qt::SkipEmptyParts).count();
            }
            info.numplayers = players;
        }
    }

    m_queryInfo[serverPath] = info;

    int idx = m_serverList->currentRow();
    if (idx >= 0)
    {
        QString currentPath = m_serverList->item(idx)->data(Qt::UserRole).toString();
        if (currentPath == serverPath)
        {
            updateQueryInfoDisplay(serverPath);
        }
    }

    updateServerListStatus();
}

void MainWindow::updateQueryInfoDisplay(const QString &serverPath)
{
    if (!m_queryInfo.contains(serverPath) || !m_queryInfo[serverPath].valid)
    {
        m_hostnameLabel->setText("Not running or no data");
        m_mapLabel->setText("N/A");
        m_gametypeLabel->setText("N/A");
        m_variantLabel->setText("N/A");
        m_playersLabel->setText("N/A");
        m_teamplayLabel->setText("N/A");
        m_fraglimitLabel->setText("N/A");
        return;
    }
    const ServerQueryInfo &info = m_queryInfo[serverPath];
    m_hostnameLabel->setText(info.hostname.isEmpty() ? "N/A" : info.hostname);
    m_mapLabel->setText(info.mapname.isEmpty() ? "N/A" : info.mapname);
    m_gametypeLabel->setText(info.gametype.isEmpty() ? "N/A" : info.gametype);
    m_variantLabel->setText(info.gamevariant.isEmpty() ? "N/A" : info.gamevariant);
    m_playersLabel->setText(QString("%1/%2").arg(info.numplayers).arg(info.maxplayers));
    m_teamplayLabel->setText(info.teamplay ? "Yes" : "No");
    m_fraglimitLabel->setText(QString::number(info.fraglimit));
}

void MainWindow::onServerLog(const QString &serverPath, const QString &line, bool isError)
{
    ConsoleWidget *console = getConsoleForServer(serverPath);
    console->appendLog(line, isError);
}

void MainWindow::onServerStateChanged(const QString &serverPath, bool running)
{
    if (!running)
    {
        if (m_queryInfo.contains(serverPath))
        {
            m_queryInfo[serverPath].valid = false;
        }
        int idx = m_serverList->currentRow();
        if (idx >= 0)
        {
            QString currentPath = m_serverList->item(idx)->data(Qt::UserRole).toString();
            if (currentPath == serverPath)
            {
                updateQueryInfoDisplay(serverPath);
            }
        }
    }
    updateServerStatus();
    statusBar()->showMessage(running ? "Server started: " + serverPath : "Server stopped: " + serverPath, 3000);
}

void MainWindow::onUninstallServer()
{
    int idx = m_serverList->currentRow();
    if (idx < 0)
    {
        QMessageBox::warning(this, "No Server Selected", "Please select a server to uninstall.");
        return;
    }

    QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
    QString type = m_serverList->item(idx)->data(Qt::UserRole + 1).toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Uninstall Server",
        QString("Are you sure you want to uninstall the server at:\n%1\n\nThis will permanently delete all files in that folder.")
            .arg(QDir::toNativeSeparators(path)),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (m_manager && m_manager->isServerRunning(path))
    {
        m_manager->stopServer(path);
        QApplication::processEvents();
        QThread::sleep(1);
    }

    m_settings->removeServer(path);
    m_settings->save();

    QDir dir(path);
    if (dir.exists())
    {
        if (!dir.removeRecursively())
        {
            QMessageBox::warning(this, "Uninstall Failed",
                                 "Could not delete the server folder. Please check permissions and try again.");
            return;
        }
    }

    if (m_consoles.contains(path))
    {
        delete m_consoles.take(path);
    }
    if (m_serverDetailWidgets.contains(path))
    {
        QWidget *w = m_serverDetailWidgets.take(path);
        if (m_contentStack->indexOf(w) != -1)
            m_contentStack->removeWidget(w);
        w->deleteLater();
    }
    m_startButtons.remove(path);
    m_stopButtons.remove(path);
    m_restartButtons.remove(path);
    m_queryInfo.remove(path);
    if (m_pendingQueries.contains(path))
    {
        m_pendingQueries[path]->deleteLater();
        m_pendingQueries.remove(path);
    }

    refreshServerList();
    updateInstalledStatus();
    statusBar()->showMessage("Server uninstalled: " + path, 3000);
}

void MainWindow::createTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);

    QPixmap pixmap(":/icons/app_icon.png");
    if (!pixmap.isNull()) {
        m_trayIcon->setIcon(QIcon(pixmap));
    } else {
        m_trayIcon->setIcon(QIcon(":/icons/app_icon.png"));
    }

    m_trayMenu = new QMenu(this);
    QAction *showAction = new QAction("Show", this);
    connect(showAction, &QAction::triggered, this, &QMainWindow::show);
    m_trayMenu->addAction(showAction);

    QAction *quitAction = new QAction("Quit", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    m_trayMenu->addAction(quitAction);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    m_trayIcon->show();

    setWindowIcon(QIcon(":/icons/app_icon.png"));
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
    {
        show();
        raise();
        activateWindow();
    }
}

void MainWindow::setStatusText(const QString &text)
{
    statusBar()->showMessage(text);
}

void MainWindow::updateInstalledStatus()
{
    QString basePath = m_installPathEdit->text();
    if (basePath.isEmpty())
    {
        m_installStatusLabel->setText("");
        return;
    }
    QString type = m_serverTypeCombo->currentText();
    QString serverFolder = QDir(basePath).absoluteFilePath(type);
    bool installed = QDir(serverFolder).exists();
    if (installed)
    {
        m_installStatusLabel->setText(QStringLiteral("✅ Installed"));
        m_installStatusLabel->setStyleSheet("color: green;");
    }
    else
    {
        m_installStatusLabel->setText(QStringLiteral("❌ Not installed"));
        m_installStatusLabel->setStyleSheet("color: gray;");
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox aboutBox;
    aboutBox.setWindowTitle("About");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(QString(
                         "<h2>%1</h2>"
                         "<p>Version %2</p>"
                         "<p>Copyright &copy; 2026 Jericho Crosby (Chalwk)</p>"
                         "<p>Licensed under the GPL License.</p>"
                         "<p>Source code: <a href='https://github.com/Chalwk/HaloServerManager'>GitHub Repository</a></p>")
                         .arg(QApplication::applicationName())
                         .arg(QApplication::applicationVersion()));
    aboutBox.exec();
}