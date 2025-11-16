/**
 * Halo Server Manager
 * Copyright (c) 2025 Jericho Crosby (Chalwk)
 * <p>
 * This project is licensed under the MIT License.
 * See LICENSE file for details:
 * https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
 */

package com.chalwk.ui;

import com.chalwk.service.FileService;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.text.*;
import java.awt.*;
import java.io.File;
import java.io.IOException;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class FileEditorDialog extends JDialog {
    // Lua keywords
    private static final Set<String> LUA_KEYWORDS = new HashSet<>();
    private static final Set<String> LUA_BUILTIN_FUNCTIONS = new HashSet<>();
    private static final Set<String> LUA_CONSTANTS = new HashSet<>();

    static {
        // Lua keywords
        String[] keywords = {
                "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
                "if", "in", "local", "nil", "not", "or", "repeat", "return", "then", "true",
                "until", "while"
        };
        Collections.addAll(LUA_KEYWORDS, keywords);

        // Lua built-in functions
        String[] builtins = {
                "print", "type", "pairs", "ipairs", "next", "tostring", "tonumber",
                "getmetatable", "setmetatable", "rawget", "rawset", "rawlen",
                "require", "dofile", "loadfile", "load", "assert", "error",
                "pcall", "xpcall", "select", "unpack", "table", "string", "math",
                "io", "os", "debug", "coroutine", "package"
        };
        Collections.addAll(LUA_BUILTIN_FUNCTIONS, builtins);

        // Lua constants
        String[] constants = {
                "nil", "true", "false"
        };
        Collections.addAll(LUA_CONSTANTS, constants);
    }

    private final File file;
    private final boolean isLuaFile;
    private JTextPane luaTextPane;
    private JTextArea plainTextArea;
    private JScrollPane scrollPane;

    public FileEditorDialog(Frame parent, File file) {
        super(parent, "Editing: " + file.getName(), true);
        this.file = file;
        this.isLuaFile = file.getName().toLowerCase().endsWith(".lua");
        initializeUI();
        loadFileContent();
    }

    private void initializeUI() {
        setLayout(new BorderLayout());
        setSize(700, 600);
        setLocationRelativeTo(getOwner());

        // Create the appropriate editor based on file type
        if (isLuaFile) {
            setupLuaEditor();
        } else {
            setupPlainEditor();
        }

        // Button panel
        JPanel buttonPanel = new JPanel(new FlowLayout());
        JButton saveButton = new JButton("Save");
        JButton cancelButton = new JButton("Cancel");

        saveButton.addActionListener(e -> saveFile());
        cancelButton.addActionListener(e -> dispose());

        buttonPanel.add(saveButton);
        buttonPanel.add(cancelButton);

        add(buttonPanel, BorderLayout.SOUTH);
    }

    private void setupPlainEditor() {
        // Plain text editor for non-Lua files
        plainTextArea = new JTextArea();
        plainTextArea.setFont(new Font("Consolas", Font.PLAIN, 13));
        plainTextArea.setTabSize(4);

        scrollPane = new JScrollPane(plainTextArea);
        add(scrollPane, BorderLayout.CENTER);
    }

    private void setupLuaEditor() {
        // Lua syntax highlighting editor
        luaTextPane = new JTextPane();
        luaTextPane.setFont(new Font("Consolas", Font.PLAIN, 13));

        // Set tab size
        TabStop[] tabs = new TabStop[5];
        for (int i = 0; i < tabs.length; i++) {
            tabs[i] = new TabStop((i + 1) * 72, TabStop.ALIGN_LEFT, TabStop.LEAD_NONE);
        }
        TabSet tabSet = new TabSet(tabs);
        SimpleAttributeSet attributes = new SimpleAttributeSet();
        StyleConstants.setTabSet(attributes, tabSet);
        luaTextPane.setParagraphAttributes(attributes, false);

        // Document listener for real-time syntax highlighting
        luaTextPane.getDocument().addDocumentListener(new DocumentListener() {
            @Override
            public void insertUpdate(DocumentEvent e) {
                SwingUtilities.invokeLater(() -> applyLuaSyntaxHighlighting());
            }

            @Override
            public void removeUpdate(DocumentEvent e) {
                SwingUtilities.invokeLater(() -> applyLuaSyntaxHighlighting());
            }

            @Override
            public void changedUpdate(DocumentEvent e) {
                // Not needed for plain text
            }
        });

        scrollPane = new JScrollPane(luaTextPane);
        add(scrollPane, BorderLayout.CENTER);
    }

    private void applyLuaSyntaxHighlighting() {
        if (luaTextPane == null) return;

        StyledDocument doc = luaTextPane.getStyledDocument();
        String text;

        try {
            text = doc.getText(0, doc.getLength());
        } catch (BadLocationException e) {
            return;
        }

        // Clear existing styles
        Style defaultStyle = doc.addStyle("default", null);
        StyleConstants.setForeground(defaultStyle, Color.BLACK);
        doc.setCharacterAttributes(0, text.length(), defaultStyle, true);

        // Create styles
        Style keywordStyle = doc.addStyle("keyword", null);
        StyleConstants.setForeground(keywordStyle, new Color(0, 0, 128)); // Dark blue
        StyleConstants.setBold(keywordStyle, true);

        Style commentStyle = doc.addStyle("comment", null);
        StyleConstants.setForeground(commentStyle, new Color(0, 128, 0)); // Green
        StyleConstants.setItalic(commentStyle, true);

        Style stringStyle = doc.addStyle("string", null);
        StyleConstants.setForeground(stringStyle, new Color(128, 0, 0)); // Dark red

        Style numberStyle = doc.addStyle("number", null);
        StyleConstants.setForeground(numberStyle, new Color(0, 0, 255)); // Blue

        Style functionStyle = doc.addStyle("function", null);
        StyleConstants.setForeground(functionStyle, new Color(128, 0, 128)); // Purple
        StyleConstants.setBold(functionStyle, true);

        Style constantStyle = doc.addStyle("constant", null);
        StyleConstants.setForeground(constantStyle, new Color(0, 128, 128)); // Teal

        // Apply highlighting patterns
        highlightPattern(doc, text, "--.*$", commentStyle); // Single line comments
        highlightPattern(doc, text, "--\\[\\[.*?\\]\\]", commentStyle); // Multi-line comments
        highlightPattern(doc, text, "\"[^\"]*\"", stringStyle); // Double quoted strings
        highlightPattern(doc, text, "'[^']*'", stringStyle); // Single quoted strings
        highlightPattern(doc, text, "\\b[0-9]+(\\.[0-9]+)?\\b", numberStyle); // Numbers

        // Highlight keywords
        for (String keyword : LUA_KEYWORDS) {
            highlightWord(doc, text, "\\b" + keyword + "\\b", keywordStyle);
        }

        // Highlight built-in functions
        for (String builtin : LUA_BUILTIN_FUNCTIONS) {
            highlightWord(doc, text, "\\b" + builtin + "\\b", functionStyle);
        }

        // Highlight constants
        for (String constant : LUA_CONSTANTS) {
            highlightWord(doc, text, "\\b" + constant + "\\b", constantStyle);
        }

        // Highlight function definitions
        highlightPattern(doc, text, "\\bfunction\\s+([a-zA-Z_][a-zA-Z0-9_]*)", functionStyle);
    }

    private void highlightPattern(StyledDocument doc, String text, String pattern, Style style) {
        try {
            Pattern p = Pattern.compile(pattern, Pattern.MULTILINE);
            Matcher m = p.matcher(text);

            while (m.find()) {
                doc.setCharacterAttributes(m.start(), m.end() - m.start(), style, false);
            }
        } catch (Exception e) {
            // Ignore pattern errors
        }
    }

    private void highlightWord(StyledDocument doc, String text, String wordPattern, Style style) {
        try {
            Pattern pattern = Pattern.compile(wordPattern);
            Matcher matcher = pattern.matcher(text);
            while (matcher.find()) {
                doc.setCharacterAttributes(matcher.start(), matcher.end() - matcher.start(), style, false);
            }
        } catch (Exception e) {
            // Ignore pattern errors
        }
    }

    private void loadFileContent() {
        try {
            String content = FileService.readFileContent(file);
            if (isLuaFile) {
                luaTextPane.setText(content);
                // Apply initial syntax highlighting
                SwingUtilities.invokeLater(this::applyLuaSyntaxHighlighting);
            } else {
                plainTextArea.setText(content);
            }
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this,
                    "Error reading file: " + e.getMessage(),
                    "Error", JOptionPane.ERROR_MESSAGE);
            dispose();
        }
    }

    private void saveFile() {
        try {
            String content;
            if (isLuaFile) {
                content = luaTextPane.getText();
            } else {
                content = plainTextArea.getText();
            }

            FileService.writeFileContent(file, content);
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