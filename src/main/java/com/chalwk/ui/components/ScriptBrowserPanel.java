package com.chalwk.ui.components;

import com.chalwk.model.ScriptCategory;
import com.chalwk.model.ScriptMetadata;
import com.chalwk.model.ServerConfig;
import com.chalwk.model.ServerType;
import com.chalwk.service.ScriptService;
import com.chalwk.service.ServerService;
import com.chalwk.ui.MainFrame;

import javax.swing.*;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class ScriptBrowserPanel extends JPanel {
    private final MainFrame parent;
    private List<ScriptMetadata> allScripts;
    private Map<ScriptCategory, List<ScriptMetadata>> scriptsByCategory;

    private JComboBox<ScriptCategory> categoryComboBox;
    private JList<ScriptMetadata> scriptList;
    private JTextArea descriptionArea;
    private JProgressBar progressBar;
    private JLabel statusLabel;
    private JButton installButton;
    private JButton viewOnGitHubButton;
    private JComboBox<ServerType> serverComboBox;
    private JButton reportBugButton;

    public ScriptBrowserPanel(MainFrame parent) {
        this.parent = parent;
        initializeUI();
        loadScripts();
    }

    private static String getString(ScriptMetadata script, File luaFolder) {
        String scriptNameWithoutExtension = script.getFilename().replace(".lua", "");
        String initFilePath = new File(luaFolder.getParentFile().getParentFile(), "sapp/init.txt").getAbsolutePath();

        return "Script '" + script.getTitle() + "' installed successfully!\n\n" +
                "Location: " + new File(luaFolder, script.getFilename()).getAbsolutePath() + "\n\n" +
                "To enable this script, add the following line to:\n" +
                initFilePath + "\n\n" +
                "Add this line:\n" +
                "lua_load \"" + scriptNameWithoutExtension + "\"\n\n" +
                "You can edit this file directly in the File Browser tab.";
    }

    private void initializeUI() {
        setLayout(new BorderLayout(10, 10));
        setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));

        // Top panel with controls
        add(createControlPanel(), BorderLayout.NORTH);

        // Center panel with script list and description
        add(createContentPanel(), BorderLayout.CENTER);

        // Status panel at bottom
        add(createStatusPanel(), BorderLayout.SOUTH);
    }

    private JPanel createControlPanel() {
        JPanel panel = new JPanel(new FlowLayout(FlowLayout.LEFT));

        // Server selection
        panel.add(new JLabel("Install to:"));
        serverComboBox = new JComboBox<>(ServerType.values());

        // Set preferred size to ensure full text is visible
        Dimension comboBoxSize = new Dimension(120, serverComboBox.getPreferredSize().height);
        serverComboBox.setPreferredSize(comboBoxSize);
        serverComboBox.setMinimumSize(comboBoxSize);
        serverComboBox.setMaximumSize(comboBoxSize);

        panel.add(serverComboBox);

        // Category filter
        panel.add(Box.createHorizontalStrut(20));
        panel.add(new JLabel("Category:"));
        categoryComboBox = new JComboBox<>(ScriptCategory.values());
        categoryComboBox.addActionListener(e -> filterScripts());
        panel.add(categoryComboBox);

        return panel;
    }

    private JSplitPane createContentPanel() {
        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        splitPane.setDividerLocation(300);
        splitPane.setResizeWeight(0.3);

        // Left panel - script list
        JPanel leftPanel = new JPanel(new BorderLayout());
        leftPanel.setBorder(BorderFactory.createTitledBorder("Available Scripts"));

        scriptList = new JList<>();
        scriptList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        scriptList.setCellRenderer(new ScriptListRenderer());
        scriptList.addListSelectionListener(new ScriptSelectionListener());

        JScrollPane listScrollPane = new JScrollPane(scriptList);
        leftPanel.add(listScrollPane, BorderLayout.CENTER);

        // Right panel - script details
        JPanel rightPanel = new JPanel(new BorderLayout());
        rightPanel.setBorder(BorderFactory.createTitledBorder("Script Details"));

        // Description area
        descriptionArea = new JTextArea();
        descriptionArea.setEditable(false);
        descriptionArea.setLineWrap(true);
        descriptionArea.setWrapStyleWord(true);
        descriptionArea.setFont(new Font("SansSerif", Font.PLAIN, 12));
        JScrollPane descriptionScrollPane = new JScrollPane(descriptionArea);

        // Button panel
        JPanel buttonPanel = new JPanel(new FlowLayout());
        installButton = new JButton("Install Script");
        viewOnGitHubButton = new JButton("View on GitHub");
        reportBugButton = new JButton("Report Bug");

        installButton.setEnabled(false);
        viewOnGitHubButton.setEnabled(false);
        reportBugButton.setEnabled(false);

        installButton.addActionListener(new InstallButtonListener());
        viewOnGitHubButton.addActionListener(new ViewOnGitHubListener());
        reportBugButton.addActionListener(new ReportBugListener());

        buttonPanel.add(installButton);
        buttonPanel.add(viewOnGitHubButton);
        buttonPanel.add(reportBugButton);

        rightPanel.add(descriptionScrollPane, BorderLayout.CENTER);
        rightPanel.add(buttonPanel, BorderLayout.SOUTH);

        splitPane.setLeftComponent(leftPanel);
        splitPane.setRightComponent(rightPanel);

        return splitPane;
    }

    private JPanel createStatusPanel() {
        JPanel panel = new JPanel(new BorderLayout());

        statusLabel = new JLabel("Loading scripts from GitHub...");
        progressBar = new JProgressBar();
        progressBar.setStringPainted(true);
        progressBar.setVisible(false);

        panel.add(statusLabel, BorderLayout.NORTH);
        panel.add(progressBar, BorderLayout.CENTER);

        return panel;
    }

    private void loadScripts() {
        new SwingWorker<Void, Void>() {
            @Override
            protected Void doInBackground() throws Exception {
                SwingUtilities.invokeLater(() -> {
                    statusLabel.setText("Loading scripts from GitHub...");
                    progressBar.setIndeterminate(true);
                });

                allScripts = ScriptService.loadScriptsMetadata();
                organizeScriptsByCategory();

                return null;
            }

            @Override
            protected void done() {
                SwingUtilities.invokeLater(() -> {
                    progressBar.setIndeterminate(false);
                    if (allScripts != null && !allScripts.isEmpty()) {
                        statusLabel.setText("Loaded " + allScripts.size() + " scripts from GitHub");
                        filterScripts();
                    } else {
                        statusLabel.setText("Failed to load scripts from GitHub");
                    }
                });
            }
        }.execute();
    }

    private void organizeScriptsByCategory() {
        scriptsByCategory = allScripts.stream()
                .collect(Collectors.groupingBy(ScriptMetadata::getCategory));
    }

    private void filterScripts() {
        if (scriptsByCategory == null) return;

        ScriptCategory selectedCategory = (ScriptCategory) categoryComboBox.getSelectedItem();
        List<ScriptMetadata> categoryScripts = scriptsByCategory.getOrDefault(selectedCategory, new ArrayList<>());

        // Sort by title
        categoryScripts.sort(Comparator.comparing(ScriptMetadata::getTitle));

        scriptList.setListData(categoryScripts.toArray(new ScriptMetadata[0]));

        // Clear selection
        scriptList.clearSelection();
        descriptionArea.setText("");
        installButton.setEnabled(false);
        viewOnGitHubButton.setEnabled(false);
    }

    private ServerConfig getServerConfig(ServerType serverType) {
        // This would need to be implemented to get the server config from MainFrame
        // For now, we'll use a simple approach.
        // I can't be bothered to implement this right now.
        String installPath = parent.getPreferencesManager().getInstallationPath(serverType.name());
        if (installPath != null) {
            File installDir = new File(installPath);
            return ServerService.detectServerConfig(serverType, installDir);
        }
        return null;
    }

    private void installScript(ScriptMetadata script, File luaFolder) {
        new Thread(() -> {
            SwingUtilities.invokeLater(() -> {
                installButton.setEnabled(false);
                statusLabel.setText("Downloading " + script.getFilename() + "...");
                progressBar.setVisible(true);
                progressBar.setValue(0);
            });

            boolean success = ScriptService.downloadScript(script, luaFolder, progressBar, statusLabel);

            SwingUtilities.invokeLater(() -> {
                installButton.setEnabled(true);
                progressBar.setVisible(false);
                if (success) {
                    statusLabel.setText("Successfully installed " + script.getFilename());

                    // Refresh the file tree in the server panel
                    parent.refreshFileTrees();

                    // Show instructions for loading the script
                    String message = getString(script, luaFolder);

                    JOptionPane.showMessageDialog(ScriptBrowserPanel.this,
                            message,
                            "Installation Complete - Next Steps", JOptionPane.INFORMATION_MESSAGE);
                }
            });
        }).start();
    }

    private static class ScriptListRenderer extends DefaultListCellRenderer {
        @Override
        public Component getListCellRendererComponent(JList<?> list, Object value, int index,
                                                      boolean isSelected, boolean cellHasFocus) {
            super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);

            if (value instanceof ScriptMetadata) {
                ScriptMetadata script = (ScriptMetadata) value;
                setText("<html><b>" + script.getTitle() + "</b><br>" +
                        script.getShortDescription() + "</html>");
            }

            return this;
        }
    }

    private class ScriptSelectionListener implements ListSelectionListener {
        @Override
        public void valueChanged(ListSelectionEvent e) {
            if (e.getValueIsAdjusting()) return;

            ScriptMetadata selectedScript = scriptList.getSelectedValue();
            if (selectedScript != null) {
                descriptionArea.setText(buildDescriptionText(selectedScript));
                installButton.setEnabled(true);
                viewOnGitHubButton.setEnabled(true);
                reportBugButton.setEnabled(true);
            } else {
                descriptionArea.setText("");
                installButton.setEnabled(false);
                viewOnGitHubButton.setEnabled(false);
                reportBugButton.setEnabled(false);
            }
        }

        private String buildDescriptionText(ScriptMetadata script) {
            return "Title: " + script.getTitle() + "\n\n" +
                    "Category: " + script.getCategory().getDisplayName() + "\n\n" +
                    "Filename: " + script.getFilename() + "\n\n" +
                    "Description:\n" + script.getDescription();
        }
    }

    private class ReportBugListener implements ActionListener {
        @Override
        public void actionPerformed(ActionEvent e) {
            ScriptMetadata selectedScript = scriptList.getSelectedValue();
            if (selectedScript != null) {
                try {

                    String fileName = selectedScript.getFilename();
                    String body = getString(selectedScript, fileName);

                    String issueTitle = URLEncoder.encode("Bug Report: " + fileName, StandardCharsets.UTF_8);
                    String issueBody = URLEncoder.encode(body, StandardCharsets.UTF_8);

                    String githubUrl = "https://github.com/Chalwk/HALO-SCRIPT-PROJECTS/issues/new?"
                            + "title=" + issueTitle
                            + "&body=" + issueBody
                            + "&labels=bug";

                    Desktop.getDesktop().browse(new java.net.URI(githubUrl));

                } catch (Exception ex) {
                    JOptionPane.showMessageDialog(ScriptBrowserPanel.this,
                            "Failed to open bug report page: " + ex.getMessage(),
                            "Error", JOptionPane.ERROR_MESSAGE);
                }
            }
        }

        private String getString(ScriptMetadata selectedScript, String fileName) {
            String category = selectedScript.getCategory().getDisplayName();
            String scriptURL = selectedScript.getGitHubUrl();

            // Build clean Markdown
            return "**Script:** " + selectedScript.getTitle() + "\n" +
                    "**Category:** " + category + "\n" +
                    "**Filename:** " + fileName + "\n" +
                    "**GitHub URL:** " + scriptURL + "\n\n" +
                    "## Bug Description\n" +
                    "Please describe the bug you encountered:\n\n" +
                    "## Steps to Reproduce\n" +
                    "1. \n2. \n3. \n\n" +
                    "## Expected Behavior\n" +
                    "What should happen?\n\n" +
                    "## Actual Behavior\n" +
                    "What actually happens?\n\n" +
                    "## Additional Context\n" +
                    "Add any other context about the problem here.\n\n" +
                    "---\n" +
                    "*Reported via Halo Server Manager*";
        }
    }


    private class InstallButtonListener implements ActionListener {
        @Override
        public void actionPerformed(ActionEvent e) {
            ScriptMetadata selectedScript = scriptList.getSelectedValue();
            ServerType selectedServer = (ServerType) serverComboBox.getSelectedItem();

            if (selectedScript == null) return;

            // Get server configuration
            assert selectedServer != null;
            ServerConfig serverConfig = getServerConfig(selectedServer);
            if (serverConfig == null || !serverConfig.isInstalled()) {
                JOptionPane.showMessageDialog(ScriptBrowserPanel.this,
                        selectedServer.getDisplayName() + " is not installed. Please install the server first.",
                        "Server Not Installed", JOptionPane.WARNING_MESSAGE);
                return;
            }

            File luaFolder = new File(serverConfig.getServerDirectory(), "cg/sapp/lua");
            if (!luaFolder.exists()) {
                luaFolder.mkdirs();
            }

            File scriptFile = new File(luaFolder, selectedScript.getFilename());
            if (scriptFile.exists()) {
                int result = JOptionPane.showConfirmDialog(ScriptBrowserPanel.this,
                        "Script '" + selectedScript.getFilename() + "' already exists. Overwrite?",
                        "Confirm Overwrite", JOptionPane.YES_NO_OPTION);
                if (result != JOptionPane.YES_OPTION) {
                    return;
                }
            }

            // Download the script
            installScript(selectedScript, luaFolder);
        }
    }

    private class ViewOnGitHubListener implements ActionListener {
        @Override
        public void actionPerformed(ActionEvent e) {
            ScriptMetadata selectedScript = scriptList.getSelectedValue();
            if (selectedScript != null) {
                try {
                    Desktop.getDesktop().browse(new java.net.URI(selectedScript.getGitHubUrl()));
                } catch (Exception ex) {
                    JOptionPane.showMessageDialog(ScriptBrowserPanel.this,
                            "Failed to open browser: " + ex.getMessage(),
                            "Error", JOptionPane.ERROR_MESSAGE);
                }
            }
        }
    }
}