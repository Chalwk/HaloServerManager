package com.chalwk.ui.components;

import com.chalwk.model.ServerConfig;
import com.chalwk.model.ServerType;
import com.chalwk.service.DownloadService;
import com.chalwk.service.FileService;
import com.chalwk.service.ServerService;
import com.chalwk.ui.FileEditorDialog;
import com.chalwk.ui.MainFrame;
import com.chalwk.util.PreferencesManager;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreePath;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;

public class ServerPanel extends JPanel {
    private MainFrame parent;
    private ServerType serverType;
    private ServerConfig serverConfig;
    private PreferencesManager preferencesManager;

    private JLabel statusLabel;
    private JProgressBar progressBar;
    private JButton downloadButton;
    private JButton launchButton;
    private JButton browseButton;
    private JTree fileTree;

    public ServerPanel(ServerType serverType, MainFrame parent, PreferencesManager preferencesManager) {
        this.serverType = serverType;
        this.parent = parent;
        this.preferencesManager = preferencesManager;
        initializeUI();
        refreshServerStatus();
    }

    public void setServerConfig(ServerConfig config) {
        this.serverConfig = config;
        refreshServerStatus();
    }

    private void initializeUI() {
        setLayout(new BorderLayout());

        // Top panel with controls
        add(createControlPanel(), BorderLayout.NORTH);

        // File tree in center
        add(createFileTreePanel(), BorderLayout.CENTER);

        // Status panel at bottom
        add(createStatusPanel(), BorderLayout.SOUTH);
    }

    private JPanel createControlPanel() {
        JPanel panel = new JPanel(new FlowLayout());

        downloadButton = new JButton("Download & Install");
        launchButton = new JButton("Launch Server");
        browseButton = new JButton("Browse Installation Directory");

        downloadButton.addActionListener(e -> downloadServer());
        launchButton.addActionListener(e -> launchServer());
        browseButton.addActionListener(e -> browseFiles());

        panel.add(downloadButton);
        panel.add(launchButton);
        panel.add(browseButton);

        return panel;
    }

    private JPanel createFileTreePanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Server Files"));

        fileTree = new JTree();
        fileTree.setRootVisible(false);
        fileTree.setShowsRootHandles(true);

        fileTree.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    TreePath path = fileTree.getPathForLocation(e.getX(), e.getY());
                    if (path != null) {
                        File file = FileService.getFileFromTreePath(path);
                        if (file != null && file.isFile() && FileService.isEditableFile(file)) {
                            openFileEditor(file);
                        }
                    }
                }
            }
        });

        JScrollPane scrollPane = new JScrollPane(fileTree);
        panel.add(scrollPane, BorderLayout.CENTER);

        return panel;
    }

    private JPanel createStatusPanel() {
        JPanel panel = new JPanel(new BorderLayout());

        statusLabel = new JLabel("Please select an installation directory");
        progressBar = new JProgressBar(0, 100);
        progressBar.setStringPainted(true);

        panel.add(statusLabel, BorderLayout.NORTH);
        panel.add(progressBar, BorderLayout.CENTER);

        return panel;
    }

    private void downloadServer() {
        File installDir = parent.getInstallDirectory();
        if (installDir == null) return;

        serverConfig = new ServerConfig(serverType, installDir, false);

        // Save to preferences
        parent.getPreferencesManager().setInstallationPath(serverType.name(), installDir.getAbsolutePath());

        // Run download in background thread
        new Thread(() -> {
            SwingUtilities.invokeLater(() -> {
                downloadButton.setEnabled(false);
                statusLabel.setText("Starting download...");
            });

            boolean success = DownloadService.downloadAndExtract(
                    serverType, installDir, progressBar, statusLabel);

            SwingUtilities.invokeLater(() -> {
                downloadButton.setEnabled(true);
                if (success) {
                    serverConfig.setInstalled(true);

                    // Ensure all directories are created
                    ServerService.createMissingServerDirectories(serverConfig);

                    refreshFileTree();
                    parent.refreshServerStatus();
                    statusLabel.setText(serverType.getDisplayName() + " installed successfully!");
                }
            });
        }).start();
    }

    private void launchServer() {
        if (serverConfig == null || !serverConfig.isInstalled()) {
            JOptionPane.showMessageDialog(this,
                    "Server is not installed. Please download and install first.",
                    "Server Not Installed", JOptionPane.WARNING_MESSAGE);
            return;
        }

        try {
            ServerService.launchServer(serverConfig);
            JOptionPane.showMessageDialog(this,
                    "Server launched successfully!",
                    "Success", JOptionPane.INFORMATION_MESSAGE);
        } catch (Exception e) {
            JOptionPane.showMessageDialog(this,
                    "Failed to launch server: " + e.getMessage(),
                    "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private void browseFiles() {
        File installDir = parent.getInstallDirectory();
        if (installDir == null) return;

        serverConfig = ServerService.detectServerConfig(serverType, installDir);

        // Save to preferences
        preferencesManager.setInstallationPath(serverType.name(), installDir.getAbsolutePath());

        refreshFileTree();
        parent.refreshServerStatus();
    }

    private void refreshFileTree() {
        if (serverConfig != null && serverConfig.isInstalled()) {
            File serverDir = serverConfig.getServerDirectory();
            if (serverDir.exists() && serverDir.isDirectory()) {
                fileTree.setModel(new DefaultTreeModel(
                        FileService.createFileTree(serverDir)));

                // Expand the root node
                fileTree.expandRow(0);
                statusLabel.setText(serverType.getDisplayName() + " is installed at: " + serverDir.getAbsolutePath());
            } else {
                setEmptyFileTree();
            }
        } else {
            setEmptyFileTree();
        }
    }

    private void setEmptyFileTree() {
        fileTree.setModel(new DefaultTreeModel(
                new DefaultMutableTreeNode("No server files found")));

        if (serverConfig != null && serverConfig.getInstallPath() != null) {
            statusLabel.setText(serverType.getDisplayName() + " is not installed in: " + serverConfig.getInstallPath().getAbsolutePath());
        } else {
            statusLabel.setText("Please select an installation directory");
        }
    }

    private void openFileEditor(File file) {
        new FileEditorDialog(parent, file).setVisible(true);
    }

    public void refreshServerStatus() {
        if (serverConfig != null) {
            boolean installed = ServerService.isServerInstalled(serverConfig);
            serverConfig.setInstalled(installed);

            if (installed) {
                statusLabel.setText(serverType.getDisplayName() + " is installed at: " +
                        serverConfig.getServerDirectory().getAbsolutePath());
                refreshFileTree();
            } else {
                statusLabel.setText(serverType.getDisplayName() + " is not installed");
                setEmptyFileTree();
            }

            launchButton.setEnabled(installed);
        } else {
            setEmptyFileTree();
        }
    }
}