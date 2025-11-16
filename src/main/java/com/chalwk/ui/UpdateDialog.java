package com.chalwk.ui;

import com.chalwk.model.UpdateConfig;
import com.chalwk.service.UpdateService;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class UpdateDialog extends JDialog {
    private final UpdateConfig updateConfig;
    private JProgressBar progressBar;
    private JLabel statusLabel;
    private JButton installButton;
    private JButton laterButton;

    public UpdateDialog(Frame parent, UpdateConfig updateConfig) {
        super(parent, "Update Available", true);
        this.updateConfig = updateConfig;
        initializeUI();
    }

    private void initializeUI() {
        setLayout(new BorderLayout(10, 10));
        setSize(500, 400);
        setLocationRelativeTo(getOwner());
        setResizable(false);

        // Header
        JPanel headerPanel = new JPanel(new BorderLayout());
        headerPanel.setBorder(new EmptyBorder(10, 10, 10, 10));
        headerPanel.setBackground(new Color(70, 130, 180));

        JLabel titleLabel = new JLabel("Update Available!");
        titleLabel.setFont(new Font("Arial", Font.BOLD, 18));
        titleLabel.setForeground(Color.WHITE);
        headerPanel.add(titleLabel, BorderLayout.NORTH);

        JLabel versionLabel = new JLabel(
                "Current: v" + updateConfig.getCurrentVersion() +
                        " → Latest: v" + updateConfig.getLatestVersion());
        versionLabel.setFont(new Font("Arial", Font.PLAIN, 14));
        versionLabel.setForeground(Color.WHITE);
        headerPanel.add(versionLabel, BorderLayout.SOUTH);

        add(headerPanel, BorderLayout.NORTH);

        // Content
        JPanel contentPanel = new JPanel(new BorderLayout(10, 10));
        contentPanel.setBorder(new EmptyBorder(10, 10, 10, 10));

        // Release notes
        JTextArea notesArea = new JTextArea(updateConfig.getReleaseNotes());
        notesArea.setEditable(false);
        notesArea.setLineWrap(true);
        notesArea.setWrapStyleWord(true);
        notesArea.setFont(new Font("Arial", Font.PLAIN, 12));

        JScrollPane scrollPane = new JScrollPane(notesArea);
        scrollPane.setBorder(BorderFactory.createTitledBorder("Release Notes"));
        scrollPane.setPreferredSize(new Dimension(400, 200));

        contentPanel.add(scrollPane, BorderLayout.CENTER);

        // Progress area (initially hidden)
        JPanel progressPanel = new JPanel(new BorderLayout(5, 5));
        progressPanel.setVisible(false);

        statusLabel = new JLabel("Ready to download update...");
        progressBar = new JProgressBar(0, 100);
        progressBar.setStringPainted(true);

        progressPanel.add(statusLabel, BorderLayout.NORTH);
        progressPanel.add(progressBar, BorderLayout.CENTER);

        contentPanel.add(progressPanel, BorderLayout.SOUTH);

        add(contentPanel, BorderLayout.CENTER);

        // Buttons
        JPanel buttonPanel = new JPanel(new FlowLayout());

        installButton = new JButton("Install Update");
        laterButton = new JButton("Remind Me Later");
        JButton skipButton = new JButton("Skip This Version");

        installButton.addActionListener(new InstallButtonListener());
        laterButton.addActionListener(e -> dispose());
        skipButton.addActionListener(e -> {
            // You could save this version to be skipped in preferences
            dispose();
        });

        buttonPanel.add(installButton);
        buttonPanel.add(laterButton);
        buttonPanel.add(skipButton);

        add(buttonPanel, BorderLayout.SOUTH);
    }

    private class InstallButtonListener implements ActionListener {
        @Override
        public void actionPerformed(ActionEvent e) {
            installButton.setEnabled(false);
            laterButton.setEnabled(false);

            // Show progress panel
            ((JPanel) getContentPane().getComponent(1)).getComponent(1).setVisible(true);

            // Download in background thread
            new Thread(() -> {
                boolean success = UpdateService.downloadUpdate(
                        updateConfig.getDownloadUrl(), progressBar, statusLabel);

                SwingUtilities.invokeLater(() -> {
                    if (success) {
                        int result = JOptionPane.showConfirmDialog(UpdateDialog.this,
                                "Update downloaded successfully! The application will now restart to complete the update.\n\n" +
                                        "Click OK to continue, or Cancel to install later.",
                                "Update Ready", JOptionPane.OK_CANCEL_OPTION);

                        if (result == JOptionPane.OK_OPTION) {
                            UpdateService.createUpdateScript(new java.io.File(
                                    updateConfig.getDownloadUrl().substring(
                                            updateConfig.getDownloadUrl().lastIndexOf("/") + 1)));
                        } else {
                            dispose();
                        }
                    } else {
                        JOptionPane.showMessageDialog(UpdateDialog.this,
                                "Failed to download update. Please try again later.",
                                "Download Failed", JOptionPane.ERROR_MESSAGE);
                        dispose();
                    }
                });
            }).start();
        }
    }
}