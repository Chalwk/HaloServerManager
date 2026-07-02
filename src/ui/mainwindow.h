// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QLabel>
#include <QStackedWidget>
#include <QToolBar>
#include <QSystemTrayIcon>
#include <QMenu>

class ServerInstaller;
class ServerManager;
class Settings;
class ConsoleWidget;
class ConfigEditor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onInstallClicked();
    void onBrowseInstallPath();

    void onServerSelectionChanged();
    void refreshServerList();

    void onLaunchServer();
    void onStopServer();
    void onRestartServer();
    void onOpenConfigEditor();
    void onUninstallServer();

    void updateServerStatus();
    void onServerLog(const QString &serverPath, const QString &line, bool isError);
    void onServerStateChanged(const QString &serverPath, bool running);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void updateInstalledStatus();

private:
    void setupUi();
    void loadConfig();
    void createTrayIcon();
    void updateServerListStatus();
    ConsoleWidget *getConsoleForServer(const QString &serverPath);
    QWidget *getServerDetailWidget(const QString &serverPath);
    void showConsoleForServer(const QString &serverPath);
    void setStatusText(const QString &text);
    void updateToolbarColors();

    QTabWidget *m_tabWidget;

    QComboBox *m_serverTypeCombo;
    QLineEdit *m_installPathEdit;
    QPushButton *m_browseButton;
    QPushButton *m_installButton;
    QProgressBar *m_downloadProgress;

    QLabel *m_installProgressLabel;
    QLabel *m_installStatusLabel;
    QLabel *m_statusLabel;

    QListWidget *m_serverList;
    QStackedWidget *m_contentStack;
    QToolBar *m_toolBar;
    QAction *m_launchAction;
    QAction *m_stopAction;
    QAction *m_restartAction;

    ServerInstaller *m_installer;
    ServerManager *m_manager;
    Settings *m_settings;

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;

    QMap<QString, ConsoleWidget *> m_consoles;
    QMap<QString, QWidget *> m_serverDetailWidgets;
};

#endif