package com.chalwk.ui;

import com.chalwk.service.FileService;

import javax.swing.*;
import java.awt.*;
import java.io.File;
import java.io.IOException;

public class FileEditorDialog extends JDialog {
    private File file;
    private JTextArea textArea;
    private JButton saveButton;
    private JButton cancelButton;

    public FileEditorDialog(Frame parent, File file) {
        super(parent, "Editing: " + file.getName(), true);
        this.file = file;
        initializeUI();
        loadFileContent();
    }

    private void initializeUI() {
        setLayout(new BorderLayout());
        setSize(600, 500);
        setLocationRelativeTo(getOwner());

        // Text area
        textArea = new JTextArea();
        textArea.setFont(new Font("Consolas", Font.PLAIN, 12));
        JScrollPane scrollPane = new JScrollPane(textArea);
        add(scrollPane, BorderLayout.CENTER);

        // Button panel
        JPanel buttonPanel = new JPanel(new FlowLayout());
        saveButton = new JButton("Save");
        cancelButton = new JButton("Cancel");

        saveButton.addActionListener(e -> saveFile());
        cancelButton.addActionListener(e -> dispose());

        buttonPanel.add(saveButton);
        buttonPanel.add(cancelButton);

        add(buttonPanel, BorderLayout.SOUTH);
    }

    private void loadFileContent() {
        try {
            String content = FileService.readFileContent(file);
            textArea.setText(content);
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this,
                    "Error reading file: " + e.getMessage(),
                    "Error", JOptionPane.ERROR_MESSAGE);
            dispose();
        }
    }

    private void saveFile() {
        try {
            FileService.writeFileContent(file, textArea.getText());
            JOptionPane.showMessageDialog(this,
                    "File saved successfully!",
                    "Success", JOptionPane.INFORMATION_MESSAGE);
            dispose();
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this,
                    "Error saving file: " + e.getMessage(),
                    "Error", JOptionPane.ERROR_MESSAGE);
        }
    }
}