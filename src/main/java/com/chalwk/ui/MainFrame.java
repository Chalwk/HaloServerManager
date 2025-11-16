/**
 * Halo Server Manager
 * Copyright (c) 2025 Jericho Crosby (Chalwk)
 * <p>
 * This project is licensed under the MIT License.
 * See LICENSE file for details:
 * https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
 */

package com.chalwk.ui;

import com.chalwk.model.ServerType;
import com.chalwk.model.UpdateConfig;
import com.chalwk.service.ServerService;
import com.chalwk.service.UpdateService;
import com.chalwk.ui.components.ScriptBrowserPanel;
import com.chalwk.ui.components.ServerPanel;
import com.chalwk.util.PreferencesManager;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

public class MainFrame extends JFrame {
    private final PreferencesManager preferencesManager;
    private ServerPanel hpcPanel;
    private ServerPanel hcePanel;
    private JMenuItem updateMenuItem;

    public MainFrame() {
        preferencesManager = new PreferencesManager();
        initializeUI();
        loadPreviousConfigurations();
        checkForUpdatesOnStartup();
    }

    public void refreshFileTrees() {
        hpcPanel.refreshFileTree();
        hcePanel.refreshFileTree();
    }

    private void initializeUI() {
        setTitle("Halo Server Manager");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setMinimumSize(new Dimension(1000, 800));

        JMenuBar menuBar = new JMenuBar();
        JMenu helpMenu = new JMenu("Help");

        updateMenuItem = new JMenuItem("Check for Updates");
        JMenuItem aboutMenuItem = new JMenuItem("About");

        updateMenuItem.addActionListener(new UpdateActionListener());
        aboutMenuItem.addActionListener(e -> showAboutDialog());

        helpMenu.add(updateMenuItem);
        helpMenu.addSeparator();
        helpMenu.add(aboutMenuItem);

        menuBar.add(helpMenu);
        setJMenuBar(menuBar);

        // Create main panel with border layout
        JPanel mainPanel = new JPanel(new BorderLayout());

        // Create tabbed pane
        JTabbedPane tabbedPane = new JTabbedPane();

        // Create server panels
        hpcPanel = new ServerPanel(ServerType.HPC, this, preferencesManager);
        hcePanel = new ServerPanel(ServerType.HCE, this, preferencesManager);

        // Create script browser panel
        ScriptBrowserPanel scriptBrowserPanel = new ScriptBrowserPanel(this);

        tabbedPane.addTab("Halo PC Server", hpcPanel);
        tabbedPane.addTab("Halo CE Server", hcePanel);
        tabbedPane.addTab("Script Browser", scriptBrowserPanel);

        mainPanel.add(tabbedPane, BorderLayout.CENTER);

        // Copyright label at bottom
        JLabel copyrightLabel = new JLabel(
                "© 2025 Halo Server Manager - Jericho Crosby (Chalwk). All rights reserved.",
                SwingConstants.CENTER
        );
        copyrightLabel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        copyrightLabel.setFont(copyrightLabel.getFont().deriveFont(Font.ITALIC));
        mainPanel.add(copyrightLabel, BorderLayout.SOUTH);

        add(mainPanel);

        // Center on screen
        pack();
        setLocationRelativeTo(null);
    }

    private void checkForUpdatesOnStartup() {
        // Check if we should check for updates on startup (configurable)
        boolean checkOnStartup = preferencesManager.getAutoUpdateEnabled();

        if (checkOnStartup) {
            // Delay the check to let the UI load first
            Timer timer = new Timer(3000, e -> {
                new Thread(() -> {
                    try {
                        UpdateConfig updateConfig = UpdateService.checkForUpdates();

                        if (updateConfig.isUpdateAvailable()) {
                            SwingUtilities.invokeLater(() -> {
                                new UpdateDialog(this, updateConfig).setVisible(true);
                            });
                        } else {
                            System.out.println("No updates available. Current: " +
                                    updateConfig.getCurrentVersion() + ", Latest: " +
                                    updateConfig.getLatestVersion());
                        }
                    } catch (Exception ex) {
                        System.err.println("Update check failed: " + ex.getMessage());
                        // Don't show error to user for automatic checks
                    }
                }).start();
            });
            timer.setRepeats(false);
            timer.start();
        }
    }

    private void checkForUpdates() {
        updateMenuItem.setEnabled(false);
        final JDialog checkingDialog = new JDialog(this, "Checking for Updates", true);

        SwingUtilities.invokeLater(() -> {
            checkingDialog.setLayout(new BorderLayout());
            checkingDialog.add(new JLabel("Checking for updates...", JLabel.CENTER), BorderLayout.CENTER);
            checkingDialog.setSize(250, 100);
            checkingDialog.setLocationRelativeTo(this);
            checkingDialog.setVisible(true);
        });

        new Thread(() -> {
            try {
                UpdateConfig updateConfig = UpdateService.checkForUpdates();

                SwingUtilities.invokeLater(() -> {
                    checkingDialog.dispose();
                    updateMenuItem.setEnabled(true);

                    if (updateConfig.isUpdateAvailable()) {
                        new UpdateDialog(this, updateConfig).setVisible(true);
                    } else {
                        JOptionPane.showMessageDialog(this,
                                "You are running the latest version (v" +
                                        updateConfig.getCurrentVersion() + ")!",
                                "No Updates Available",
                                JOptionPane.INFORMATION_MESSAGE);
                    }
                });
            } catch (Exception e) {
                SwingUtilities.invokeLater(() -> {
                    checkingDialog.dispose();
                    updateMenuItem.setEnabled(true);

                    JOptionPane.showMessageDialog(this,
                            "Failed to check for updates: " + e.getMessage() +
                                    "\n\nPlease check your internet connection and try again.",
                            "Update Check Failed",
                            JOptionPane.WARNING_MESSAGE);
                });
            }
        }).start();
    }

    private void showAboutDialog() {
        String aboutText =
                "<html><center>" +
                        "<h2>Halo Server Manager</h2>" +
                        "<p>Version: 1.0.0</p>" +
                        "<p>© 2025 Jericho Crosby (Chalwk)</p>" +
                        "<p>All rights reserved.</p>" +
                        "<br>" +
                        "<p>Manage Halo PC and Halo CE dedicated servers</p>" +
                        "<p>with an easy-to-use graphical interface.</p>" +
                        "</center></html>";

        JOptionPane.showMessageDialog(this, aboutText, "About", JOptionPane.INFORMATION_MESSAGE);
    }

    private void loadPreviousConfigurations() {
        // Load previously used installation directories
        String hpcPath = preferencesManager.getInstallationPath("HPC");
        String hcePath = preferencesManager.getInstallationPath("HCE");

        if (hpcPath != null) {
            File installDir = new File(hpcPath);
            if (installDir.exists()) {
                hpcPanel.setServerConfig(ServerService.detectServerConfig(ServerType.HPC, installDir));
            }
        }

        if (hcePath != null) {
            File installDir = new File(hcePath);
            if (installDir.exists()) {
                hcePanel.setServerConfig(ServerService.detectServerConfig(ServerType.HCE, installDir));
            }
        }

        refreshServerStatus();
    }

    public void refreshServerStatus() {
        hpcPanel.refreshServerStatus();
        hcePanel.refreshServerStatus();
    }

    public File getInstallDirectory() {
        // Use a common directory for both servers
        JFileChooser chooser = new JFileChooser();
        chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
        chooser.setDialogTitle("Select Installation Directory");

        // Set current directory if we have a previous selection
        String previousPath = preferencesManager.getInstallationPath("HPC");
        if (previousPath != null) {
            chooser.setCurrentDirectory(new File(previousPath));
        }

        if (chooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            return chooser.getSelectedFile();
        }
        return null;
    }

    public PreferencesManager getPreferencesManager() {
        return preferencesManager;
    }

    private class UpdateActionListener implements ActionListener {
        @Override
        public void actionPerformed(ActionEvent e) {
            updateMenuItem.setEnabled(false);

            // Show checking dialog
            JDialog checkingDialog = new JDialog(MainFrame.this, "Checking for Updates", true);
            checkingDialog.setLayout(new BorderLayout());
            checkingDialog.add(new JLabel("Checking for updates...", JLabel.CENTER), BorderLayout.CENTER);
            checkingDialog.setSize(200, 100);
            checkingDialog.setLocationRelativeTo(MainFrame.this);

            Timer timer = new Timer(100, evt -> {
                checkingDialog.setVisible(true);
            });
            timer.setRepeats(false);
            timer.start();

            new Thread(() -> {
                checkForUpdates();

                SwingUtilities.invokeLater(() -> {
                    checkingDialog.dispose();
                    updateMenuItem.setEnabled(true);
                });
            }).start();
        }
    }
}