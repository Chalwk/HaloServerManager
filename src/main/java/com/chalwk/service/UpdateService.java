/**
 * Halo Server Manager
 * Copyright (c) 2025 Jericho Crosby (Chalwk)
 * <p>
 * This project is licensed under the MIT License.
 * See LICENSE file for details:
 * https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
 */

package com.chalwk.service;

import com.chalwk.model.UpdateConfig;
import org.json.JSONObject;

import javax.swing.*;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.Properties;

public class UpdateService {

    private static final String GITHUB_REPO = "Chalwk/HaloServerManager";
    private static final String VERSION_URL = "https://api.github.com/repos/" + GITHUB_REPO + "/releases/latest";
    private static final String VERSION_FILE = "version.properties";

    public static UpdateConfig checkForUpdates() {
        try {
            String currentVersion = getCurrentVersion();
            JSONObject latestRelease = fetchLatestRelease();

            if (latestRelease != null) {
                String latestVersion = latestRelease.getString("tag_name").replace("v", "");
                String downloadUrl = getDownloadUrl(latestRelease);
                String releaseNotes = latestRelease.getString("body");

                UpdateConfig config = new UpdateConfig(currentVersion, latestVersion, downloadUrl, releaseNotes);

                // Save the latest version info
                saveVersionInfo(latestVersion, downloadUrl, releaseNotes);

                return config;
            }
        } catch (Exception e) {
            System.err.println("Error checking for updates: " + e.getMessage());
            e.printStackTrace();
        }

        return new UpdateConfig(getCurrentVersion(), getCurrentVersion(), "", "Unable to check for updates.");
    }

    private static String getCurrentVersion() {
        try (InputStream input = new FileInputStream(VERSION_FILE)) {
            Properties prop = new Properties();
            prop.load(input);
            return prop.getProperty("version", "1.0.0");
        } catch (IOException e) {
            // If version file doesn't exist, create it with default version
            saveVersionInfo("1.0.0", "", "Initial release");
            return "1.0.0";
        }
    }

    private static JSONObject fetchLatestRelease() {
        try {
            URL url = new URL(VERSION_URL);
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setRequestProperty("Accept", "application/vnd.github.v3+json");
            connection.setRequestProperty("User-Agent", "HaloServerManager");
            connection.setConnectTimeout(10000);
            connection.setReadTimeout(10000);

            int responseCode = connection.getResponseCode();
            System.out.println("GitHub API Response Code: " + responseCode);

            if (responseCode == 200) {
                try (BufferedReader reader = new BufferedReader(
                        new InputStreamReader(connection.getInputStream()))) {

                    StringBuilder content = new StringBuilder();
                    String line;
                    while ((line = reader.readLine()) != null) {
                        content.append(line);
                    }

                    return new JSONObject(content.toString());
                }
            } else {
                // Read error stream for more info
                try (BufferedReader reader = new BufferedReader(
                        new InputStreamReader(connection.getErrorStream()))) {
                    StringBuilder errorContent = new StringBuilder();
                    String line;
                    while ((line = reader.readLine()) != null) {
                        errorContent.append(line);
                    }
                    System.err.println("GitHub API Error: " + errorContent.toString());
                }
            }
        } catch (Exception e) {
            System.err.println("Failed to fetch release info: " + e.getMessage());
            e.printStackTrace();
        }
        return null;
    }

    private static String getDownloadUrl(JSONObject release) {
        try {
            var assets = release.getJSONArray("assets");
            for (int i = 0; i < assets.length(); i++) {
                JSONObject asset = assets.getJSONObject(i);
                String name = asset.getString("name");
                if (name.toLowerCase().endsWith(".exe") &&
                        name.toLowerCase().contains("haloservermanager")) {
                    return asset.getString("browser_download_url");
                }
            }

            // Fallback: return first asset if no EXE found
            if (!assets.isEmpty()) {
                return assets.getJSONObject(0).getString("browser_download_url");
            }
        } catch (Exception e) {
            System.err.println("Error finding download URL: " + e.getMessage());
            e.printStackTrace();
        }
        return "";
    }

    private static void saveVersionInfo(String version, String downloadUrl, String releaseNotes) {
        try (OutputStream output = new FileOutputStream(VERSION_FILE)) {
            Properties prop = new Properties();
            prop.setProperty("version", version);
            prop.setProperty("downloadUrl", downloadUrl);
            prop.setProperty("releaseNotes", releaseNotes);
            prop.setProperty("lastChecked", String.valueOf(System.currentTimeMillis()));
            prop.store(output, "Halo Server Manager Version Info");
        } catch (IOException e) {
            System.err.println("Failed to save version info: " + e.getMessage());
        }
    }

    public static boolean downloadUpdate(String downloadUrl, JProgressBar progressBar, JLabel statusLabel) {
        try {
            URL url = new URL(downloadUrl);
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setRequestProperty("User-Agent", "HaloServerManager");

            int responseCode = connection.getResponseCode();
            if (responseCode != HttpURLConnection.HTTP_OK) {
                SwingUtilities.invokeLater(() ->
                        statusLabel.setText("Download failed. HTTP code: " + responseCode));
                return false;
            }

            long fileSize = connection.getContentLengthLong();
            String fileName = getFileNameFromUrl(downloadUrl);
            File tempFile = new File(fileName + ".tmp");

            try (InputStream inputStream = connection.getInputStream();
                 FileOutputStream outputStream = new FileOutputStream(tempFile)) {

                byte[] buffer = new byte[4096];
                long totalBytesRead = 0;
                int bytesRead;

                while ((bytesRead = inputStream.read(buffer)) != -1) {
                    outputStream.write(buffer, 0, bytesRead);
                    totalBytesRead += bytesRead;

                    final long currentTotal = totalBytesRead;
                    final long currentFileSize = fileSize;

                    SwingUtilities.invokeLater(() -> {
                        int progress = fileSize > 0 ? (int) ((currentTotal * 100) / currentFileSize) : 0;
                        progressBar.setValue(progress);
                        statusLabel.setText(String.format("Downloading update: %d%% (%d/%d KB)",
                                progress, currentTotal / 1024, currentFileSize / 1024));
                    });
                }
            }

            connection.disconnect();

            // Rename temp file to final EXE
            File finalFile = new File(fileName);
            Files.move(tempFile.toPath(), finalFile.toPath(), StandardCopyOption.REPLACE_EXISTING);

            SwingUtilities.invokeLater(() -> {
                progressBar.setValue(100);
                statusLabel.setText("Update downloaded successfully!");
            });

            return true;

        } catch (Exception e) {
            SwingUtilities.invokeLater(() ->
                    statusLabel.setText("Download error: " + e.getMessage()));
            e.printStackTrace();
            return false;
        }
    }

    private static String getFileNameFromUrl(String url) {
        return url.substring(url.lastIndexOf("/") + 1);
    }

    public static void createUpdateScript(File newExeFile) {
        // Create a batch script that will replace the current EXE and restart
        String scriptContent = createBatchScript(newExeFile.getName());

        try {
            File scriptFile = new File("update.bat");
            try (FileWriter writer = new FileWriter(scriptFile)) {
                writer.write(scriptContent);
            }

            // Execute the script
            Runtime.getRuntime().exec("cmd /c start update.bat");

            // Exit the current application
            System.exit(0);

        } catch (IOException e) {
            JOptionPane.showMessageDialog(null,
                    "Failed to create update script: " + e.getMessage(),
                    "Update Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private static String createBatchScript(String newExeName) {
        return "@echo off\n" +
                "chcp 65001 >nul\n" +
                "echo ============================================\n" +
                "echo    Halo Server Manager - Auto Updater\n" +
                "echo ============================================\n" +
                "echo.\n" +
                "echo Updating Halo Server Manager...\n" +
                "echo Please wait while we install the new version.\n" +
                "echo.\n" +
                "timeout /t 3 /nobreak >nul\n" +
                "\n" +
                ":wait_for_close\n" +
                "echo Waiting for Halo Server Manager to close...\n" +
                "tasklist /FI \"IMAGENAME eq HaloServerManager*.exe\" 2>nul | find /I \"HaloServerManager\" >nul\n" +
                "if %errorlevel% == 0 (\n" +
                "    timeout /t 2 /nobreak >nul\n" +
                "    goto wait_for_close\n" +
                ")\n" +
                "\n" +
                "echo.\n" +
                "echo Installing new version...\n" +
                "\n" +
                "REM Delete old version if it exists\n" +
                "if exist \"HaloServerManager.exe\" (\n" +
                "    del \"HaloServerManager.exe\"\n" +
                ")\n" +
                "\n" +
                "REM Rename new version\n" +
                "if exist \"" + newExeName + "\" (\n" +
                "    ren \"" + newExeName + "\" \"HaloServerManager.exe\"\n" +
                "    echo Update successful!\n" +
                ") else (\n" +
                "    echo Error: New version file not found!\n" +
                "    pause\n" +
                "    exit /b 1\n" +
                ")\n" +
                "\n" +
                "echo.\n" +
                "echo Starting Halo Server Manager...\n" +
                "timeout /t 2 /nobreak >nul\n" +
                "\n" +
                "REM Start the updated application\n" +
                "start \"\" \"HaloServerManager.exe\"\n" +
                "\n" +
                "echo.\n" +
                "echo Update completed successfully!\n" +
                "echo This window will close automatically...\n" +
                "timeout /t 3 /nobreak >nul\n" +
                "\n" +
                "REM Clean up - delete this batch file\n" +
                "del \"%~f0\"\n" +
                "exit\n";
    }
}