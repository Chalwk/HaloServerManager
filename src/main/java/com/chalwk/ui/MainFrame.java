package com.chalwk.ui;

import com.chalwk.model.ServerType;
import com.chalwk.service.ServerService;
import com.chalwk.ui.components.ScriptBrowserPanel;
import com.chalwk.ui.components.ServerPanel;
import com.chalwk.util.PreferencesManager;

import javax.swing.*;
import java.awt.*;
import java.io.File;

public class MainFrame extends JFrame {
    private final PreferencesManager preferencesManager;
    private ServerPanel hpcPanel;
    private ServerPanel hcePanel;

    public MainFrame() {
        preferencesManager = new PreferencesManager();
        initializeUI();
        loadPreviousConfigurations();
    }

    public void refreshFileTrees() {
        hpcPanel.refreshFileTree();
        hcePanel.refreshFileTree();
    }

    private void initializeUI() {
        setTitle("Halo Server Manager");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setMinimumSize(new Dimension(1000, 800));

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
}