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
#include <QMap>
#include <QGroupBox>
#include <QSet>

class ServerInstaller;
class ServerManager;
class Settings;
class ConsoleWidget;
class ConfigEditor;
class QUdpSocket;

struct ServerQueryInfo
{
    QString hostname;
    int maxplayers = 0;
    int numplayers = 0;
    QString mapname;
    QString gametype;
    QString gamevariant;
    bool teamplay = false;
    int fraglimit = 0;
    bool valid = false;
};

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

    void onUninstallServer();

    void updateServerStatus();
    void onServerLog(const QString &serverPath, const QString &line, bool isError);
    void onServerStateChanged(const QString &serverPath, bool running);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void updateInstalledStatus();

    void showAboutDialog();

private:
    void setupUi();
    void loadConfig();
    void createTrayIcon();
    void updateServerListStatus();
    ConsoleWidget *getConsoleForServer(const QString &serverPath);
    QWidget *getServerDetailWidget(const QString &serverPath);
    void showConsoleForServer(const QString &serverPath);
    void setStatusText(const QString &text);
    void updateQueryInfoDisplay(const QString &serverPath);

    void sendServerQuery(const QString &serverPath, int port);
    void parseQueryReply(const QString &serverPath, const QByteArray &reply);

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
    QAction *m_aboutAction;

    ServerInstaller *m_installer;
    ServerManager *m_manager;
    Settings *m_settings;

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;

    QMap<QString, ConsoleWidget *> m_consoles;
    QMap<QString, QWidget *> m_serverDetailWidgets;

    QMap<QString, QPushButton *> m_startButtons;
    QMap<QString, QPushButton *> m_stopButtons;
    QMap<QString, QPushButton *> m_restartButtons;

    QMap<QString, QUdpSocket *> m_pendingQueries;

    QMap<QString, ServerQueryInfo> m_queryInfo;
    QGroupBox *m_queryInfoGroup;
    QLabel *m_hostnameLabel;
    QLabel *m_mapLabel;
    QLabel *m_gametypeLabel;
    QLabel *m_variantLabel;
    QLabel *m_playersLabel;
    QLabel *m_teamplayLabel;
    QLabel *m_fraglimitLabel;

    QSet<QString> m_runningServers;
    QSet<QString> m_stoppingServers;
};

#endif