package com.chalwk;

import com.chalwk.ui.MainFrame;

import javax.swing.*;

public class HaloServerManager {
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            try {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } catch (Exception e) {
                e.printStackTrace();
            }

            new MainFrame().setVisible(true);
        });
    }
}