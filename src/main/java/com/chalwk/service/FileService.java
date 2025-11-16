package com.chalwk.service;

import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.TreePath;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Arrays;

public class FileService {

    public static DefaultMutableTreeNode createFileTree(File rootDir) {
        DefaultMutableTreeNode root = new DefaultMutableTreeNode(rootDir.getName());

        if (rootDir.exists() && rootDir.isDirectory()) {
            addFilesToTree(root, rootDir);
        }

        return root;
    }

    private static void addFilesToTree(DefaultMutableTreeNode node, File dir) {
        File[] files = dir.listFiles();
        if (files == null) return;

        // Sort files: directories first, then files
        Arrays.sort(files, (f1, f2) -> {
            if (f1.isDirectory() && !f2.isDirectory()) return -1;
            if (!f1.isDirectory() && f2.isDirectory()) return 1;
            return f1.getName().compareToIgnoreCase(f2.getName());
        });

        for (File file : files) {
            DefaultMutableTreeNode childNode = new DefaultMutableTreeNode(new FileNode(file));
            node.add(childNode);

            if (file.isDirectory()) {
                addFilesToTree(childNode, file);
            }
        }
    }

    public static boolean isEditableFile(File file) {
        if (file == null || !file.isFile()) return false;

        String name = file.getName().toLowerCase();
        String path = file.getAbsolutePath().toLowerCase();

        // Check if it's a .txt file in sapp folders or run.bat
        if (name.equals("run.bat")) return true;
        if (name.endsWith(".txt") &&
                (path.contains("sapp") || path.contains("cg"))) return true;
        return name.endsWith(".lua") && path.contains("lua");
    }

    public static String readFileContent(File file) throws IOException {
        return Files.readString(file.toPath());
    }

    public static void writeFileContent(File file, String content) throws IOException {
        Files.writeString(file.toPath(), content);
    }

    public static File getFileFromTreePath(TreePath path) {
        if (path == null) return null;

        Object lastComponent = path.getLastPathComponent();
        if (lastComponent instanceof DefaultMutableTreeNode) {
            Object userObject = ((DefaultMutableTreeNode) lastComponent).getUserObject();
            if (userObject instanceof FileNode) {
                return ((FileNode) userObject).getFile();
            }
        }
        return null;
    }

    public static class FileNode {
        private final File file;

        public FileNode(File file) {
            this.file = file;
        }

        public File getFile() {
            return file;
        }

        @Override
        public String toString() {
            return file.getName();
        }
    }
}