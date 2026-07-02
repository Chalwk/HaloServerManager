// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "mainwindow.h"
#include "serverinstaller.h"
#include "servermanager.h"
#include "serverprocess.h"
#include "settings.h"
#include "consolewidget.h"
#include "configeditor.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_installer(nullptr), m_manager(nullptr), m_settings(new Settings(this))
{
    setWindowTitle("Halo Server Manager");
    resize(1100, 650);
    setupUi();
    loadConfig();
    createTrayIcon();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateServerStatus);
    timer->start(2000);
    updateServerStatus();
}

MainWindow::~MainWindow()
{
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
    typeLayout->addStretch();
    installLayout->addWidget(typeGroup);

    QGroupBox *pathGroup = new QGroupBox("Installation Directory");
    QHBoxLayout *pathLayout = new QHBoxLayout(pathGroup);
    m_installPathEdit = new QLineEdit();
    m_installPathEdit->setReadOnly(true);
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

    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    installLayout->addWidget(m_statusLabel);

    installLayout->addStretch();
    m_tabWidget->addTab(installTab, "Install");

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

    QPushButton *newServerBtn = new QPushButton("Install New Server");
    connect(newServerBtn, &QPushButton::clicked, this, [this]()
            { m_tabWidget->setCurrentIndex(0); });
    leftLayout->addWidget(newServerBtn);
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

    m_launchAction = new QAction("Start", this);
    m_launchAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connect(m_launchAction, &QAction::triggered, this, &MainWindow::onLaunchServer);
    m_toolBar->addAction(m_launchAction);

    m_stopAction = new QAction("Stop", this);
    m_stopAction->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    connect(m_stopAction, &QAction::triggered, this, &MainWindow::onStopServer);
    m_toolBar->addAction(m_stopAction);

    m_restartAction = new QAction("Restart", this);
    m_restartAction->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_restartAction, &QAction::triggered, this, &MainWindow::onRestartServer);
    m_toolBar->addAction(m_restartAction);

    m_launchAction->setEnabled(false);
    m_stopAction->setEnabled(false);
    m_restartAction->setEnabled(false);
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
    m_statusLabel->setText("Downloading...");

    if (!m_installer)
    {
        m_installer = new ServerInstaller(this);
        connect(m_installer, &ServerInstaller::downloadProgress, m_downloadProgress, &QProgressBar::setValue);
        connect(m_installer, &ServerInstaller::installationFinished, this, [this](bool success, const QString &message)
                {
            m_downloadProgress->setVisible(false);
            m_installButton->setEnabled(!m_installPathEdit->text().isEmpty());
            if (success) {
                m_statusLabel->setText("Installation completed successfully.");
            } else {
                m_statusLabel->setText("Installation failed: " + message);
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
        QListWidgetItem *item = new QListWidgetItem(QString("%1  [%2]").arg(QDir::toNativeSeparators(path), type));
        item->setData(Qt::UserRole, path);
        m_serverList->addItem(item);
    }
    updateServerListStatus();
    onServerSelectionChanged();
}

void MainWindow::updateServerListStatus()
{
    for (int i = 0; i < m_serverList->count(); ++i)
    {
        QListWidgetItem *item = m_serverList->item(i);
        QString path = item->data(Qt::UserRole).toString();
        bool running = m_manager && m_manager->isServerRunning(path);
        item->setText(QString("%1  [%2] %3").arg(QDir::toNativeSeparators(path), running ? "●" : "○", running ? "RUNNING" : "STOPPED"));
        if (running)
            item->setForeground(Qt::darkGreen);
        else
            item->setForeground(Qt::red);
    }
}

void MainWindow::onServerSelectionChanged()
{
    int idx = m_serverList->currentRow();
    bool hasSelection = (idx >= 0);
    m_launchAction->setEnabled(hasSelection);
    m_stopAction->setEnabled(hasSelection);
    m_restartAction->setEnabled(hasSelection);

    if (hasSelection)
    {
        QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
        showConsoleForServer(path);

        bool running = m_manager && m_manager->isServerRunning(path);
        m_launchAction->setEnabled(!running);
        m_stopAction->setEnabled(running);
        m_restartAction->setEnabled(true);
    }
    else
    {
        m_contentStack->setCurrentIndex(0);
    }
    updateServerStatus();
    updateToolbarColors();
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

QWidget *MainWindow::getServerDetailWidget(const QString &serverPath)
{
    if (m_serverDetailWidgets.contains(serverPath))
        return m_serverDetailWidgets[serverPath];

    QWidget *container = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    QTabWidget *tabWidget = new QTabWidget;

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

void MainWindow::onLaunchServer()
{
    int idx = m_serverList->currentRow();
    if (idx < 0)
        return;
    QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();

    QJsonArray servers = m_settings->servers();
    QString type;
    int port = 2302;
    for (const QJsonValue &val : servers)
    {
        QJsonObject obj = val.toObject();
        if (obj["path"].toString() == path)
        {
            type = obj["type"].toString();
            port = obj["port"].toInt(2302);
            break;
        }
    }
    if (type.isEmpty())
        return;

    ConsoleWidget *console = getConsoleForServer(path);
    if (console)
        console->clear();

    if (m_manager->launchServer(path, port, type))
    {
        statusBar()->showMessage("Server launched: " + path, 3000);
        updateServerStatus();
        showConsoleForServer(path);
    }
    else
    {
        statusBar()->showMessage("Failed to launch server", 3000);
    }
}

void MainWindow::onStopServer()
{
    int idx = m_serverList->currentRow();
    if (idx < 0)
        return;
    QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
    m_manager->stopServer(path);
    statusBar()->showMessage("Server stopped: " + path, 3000);
    updateServerStatus();
}

void MainWindow::onRestartServer()
{
    int idx = m_serverList->currentRow();
    if (idx < 0)
        return;
    QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
    m_manager->restartServer(path);
    statusBar()->showMessage("Restarting server: " + path, 3000);
    updateServerStatus();
}

void MainWindow::onOpenConfigEditor()
{
    int idx = m_serverList->currentRow();
    if (idx < 0)
        return;
    QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
    ConfigEditor editor(path, this);
    editor.exec();
}

void MainWindow::updateServerStatus()
{
    updateServerListStatus();
    int idx = m_serverList->currentRow();
    if (idx < 0)
    {
        statusBar()->showMessage("No server selected");
        return;
    }
    QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
    bool running = m_manager && m_manager->isServerRunning(path);
    m_launchAction->setEnabled(!running);
    m_stopAction->setEnabled(running);
    m_restartAction->setEnabled(true);
    ConsoleWidget *console = m_consoles.value(path, nullptr);
    if (console)
    {
        console->setRunning(running);
    }
    if (running)
    {
        ServerProcess *proc = m_manager->getProcess(path);
        if (proc)
        {
            qint64 uptime = proc->uptime();
            statusBar()->showMessage(QString("Server running for %1 seconds").arg(uptime));
        }
    }
    else
    {
        statusBar()->showMessage("Server stopped");
    }
    updateToolbarColors();
}

void MainWindow::onServerLog(const QString &serverPath, const QString &line, bool isError)
{
    ConsoleWidget *console = getConsoleForServer(serverPath);
    console->appendLog(line, isError);
}

void MainWindow::onServerStateChanged(const QString &serverPath, bool running)
{
    updateServerListStatus();
    int idx = m_serverList->currentRow();
    if (idx >= 0)
    {
        QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
        if (path == serverPath)
        {
            m_launchAction->setEnabled(!running);
            m_stopAction->setEnabled(running);
            ConsoleWidget *console = m_consoles.value(serverPath, nullptr);
            if (console)
                console->setRunning(running);
            if (running)
            {
                statusBar()->showMessage("Server started: " + serverPath, 3000);
            }
            else
            {
                statusBar()->showMessage("Server stopped: " + serverPath, 3000);
            }
        }
    }
    updateToolbarColors();
}

void MainWindow::createTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/app_icon.png"));
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

void MainWindow::updateToolbarColors()
{
    int idx = m_serverList->currentRow();
    bool hasSelection = (idx >= 0);
    bool running = false;
    if (hasSelection)
    {
        QString path = m_serverList->item(idx)->data(Qt::UserRole).toString();
        running = m_manager && m_manager->isServerRunning(path);
    }

    QWidget *launchWidget = m_toolBar->widgetForAction(m_launchAction);
    QWidget *stopWidget = m_toolBar->widgetForAction(m_stopAction);

    if (launchWidget)
    {
        if (hasSelection && !running)
        {
            launchWidget->setStyleSheet("QToolButton { color: green; }");
        }
        else
        {
            launchWidget->setStyleSheet("");
        }
    }
    if (stopWidget)
    {
        if (hasSelection && running)
        {
            stopWidget->setStyleSheet("QToolButton { color: red; }");
        }
        else
        {
            stopWidget->setStyleSheet("");
        }
    }
}