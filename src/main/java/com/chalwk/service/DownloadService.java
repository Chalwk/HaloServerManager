package com.chalwk.service;

import com.chalwk.model.ServerType;

import javax.swing.*;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class DownloadService {

    public static boolean downloadAndExtract(ServerType serverType, File targetDir,
                                             JProgressBar progressBar, JLabel statusLabel) {
        try {
            String downloadUrl = serverType.getDownloadUrl();
            String fileName = serverType.getFolderName() + ".zip";
            File zipFile = new File(targetDir, fileName);

            // Download the file
            if (!downloadFile(downloadUrl, zipFile, progressBar, statusLabel)) {
                return false;
            }

            // Extract the file
            if (!extractZipFile(zipFile, targetDir, progressBar, statusLabel)) {
                return false;
            }

            // Create missing directory structure (empty folders that aren't in ZIP)
            createMissingDirectories(serverType, targetDir);

            // Delete the zip file after extraction
            Files.deleteIfExists(zipFile.toPath());

            return true;

        } catch (Exception e) {
            SwingUtilities.invokeLater(() ->
                    statusLabel.setText("Error: " + e.getMessage()));
            return false;
        }
    }

    private static void createMissingDirectories(ServerType serverType, File targetDir) {
        File serverDir = new File(targetDir, serverType.getFolderName());

        // Create the known directory structure that might be missing from ZIP
        String[] directories = {
                "cg/sapp/lua",
                "cg/savegames",
                "sapp"
        };

        for (String dir : directories) {
            File directory = new File(serverDir, dir);
            if (!directory.exists()) {
                directory.mkdirs();
            }
        }

        // Also ensure the main directories exist
        new File(serverDir, "maps").mkdirs();
        new File(serverDir, "cg").mkdirs();
    }

    private static boolean downloadFile(String fileURL, File outputFile,
                                        JProgressBar progressBar, JLabel statusLabel) {
        try {
            URL url = new URL(fileURL);
            HttpURLConnection httpConn = (HttpURLConnection) url.openConnection();
            int responseCode = httpConn.getResponseCode();

            if (responseCode != HttpURLConnection.HTTP_OK) {
                SwingUtilities.invokeLater(() ->
                        statusLabel.setText("Download failed. Server returned HTTP code: " + responseCode));
                return false;
            }

            long fileSize = httpConn.getContentLengthLong();

            try (InputStream inputStream = httpConn.getInputStream();
                 FileOutputStream outputStream = new FileOutputStream(outputFile)) {

                byte[] buffer = new byte[4096];
                long totalBytesRead = 0;
                int bytesRead;

                while ((bytesRead = inputStream.read(buffer)) != -1) {
                    outputStream.write(buffer, 0, bytesRead);
                    totalBytesRead += bytesRead;

                    // Create effectively final copies for use in lambda
                    final long currentTotal = totalBytesRead;
                    final long currentFileSize = fileSize;

                    SwingUtilities.invokeLater(() -> {
                        int progress = (int) ((currentTotal * 100) / currentFileSize);
                        progressBar.setValue(progress);
                        statusLabel.setText(String.format("Downloading: %d%% (%d/%d KB)",
                                progress, currentTotal / 1024, currentFileSize / 1024));
                    });
                }
            }

            httpConn.disconnect();
            return true;

        } catch (Exception e) {
            SwingUtilities.invokeLater(() ->
                    statusLabel.setText("Download error: " + e.getMessage()));
            return false;
        }
    }

    private static boolean extractZipFile(File zipFile, File outputDir,
                                          JProgressBar progressBar, JLabel statusLabel) {
        try (ZipInputStream zipIn = new ZipInputStream(new FileInputStream(zipFile))) {
            ZipEntry entry;
            byte[] buffer = new byte[4096];

            while ((entry = zipIn.getNextEntry()) != null) {
                // Create effectively final copy for use in lambda
                final String currentEntryName = entry.getName();
                File filePath = new File(outputDir, currentEntryName);

                if (!entry.isDirectory()) {
                    Files.createDirectories(filePath.getParentFile().toPath());

                    try (FileOutputStream fos = new FileOutputStream(filePath)) {
                        int bytesRead;
                        while ((bytesRead = zipIn.read(buffer)) != -1) {
                            fos.write(buffer, 0, bytesRead);
                        }
                    }

                    SwingUtilities.invokeLater(() ->
                            statusLabel.setText("Extracting: " + currentEntryName));
                } else {
                    // Create directory entries
                    Files.createDirectories(filePath.toPath());
                }

                zipIn.closeEntry();
            }

            SwingUtilities.invokeLater(() -> {
                progressBar.setValue(100);
                statusLabel.setText("Extraction completed successfully!");
            });

            return true;

        } catch (Exception e) {
            SwingUtilities.invokeLater(() ->
                    statusLabel.setText("Extraction error: " + e.getMessage()));
            return false;
        }
    }
}