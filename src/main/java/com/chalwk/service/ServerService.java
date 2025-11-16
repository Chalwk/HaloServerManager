/**
 * Halo Server Manager
 * Copyright (c) 2025 Jericho Crosby (Chalwk)
 * <p>
 * This project is licensed under the MIT License.
 * See LICENSE file for details:
 * https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
 */

package com.chalwk.service;

import com.chalwk.model.ServerConfig;
import com.chalwk.model.ServerType;

import java.io.File;
import java.io.IOException;

public class ServerService {

    public static boolean isServerInstalled(ServerConfig config) {
        if (config == null || config.getInstallPath() == null) return false;

        File serverDir = config.getServerDirectory();
        File runBat = config.getRunBatFile();

        return serverDir.exists() && serverDir.isDirectory() &&
                runBat.exists() && runBat.isFile();
    }

    public static void launchServer(ServerConfig config) {
        if (!isServerInstalled(config)) {
            throw new IllegalStateException("Server is not installed");
        }

        try {
            File serverDir = config.getServerDirectory();
            ProcessBuilder pb = new ProcessBuilder("cmd.exe", "/c", "start", "run.bat");
            pb.directory(serverDir);
            pb.start();
        } catch (IOException e) {
            throw new RuntimeException("Failed to launch server: " + e.getMessage(), e);
        }
    }

    public static ServerConfig detectServerConfig(ServerType serverType, File selectedDir) {
        File serverDir;

        // Check if the selected directory IS the server directory
        if (selectedDir.getName().equals(serverType.getFolderName())) {
            serverDir = selectedDir;
        }
        // Check if the selected directory CONTAINS the server directory
        else if (new File(selectedDir, serverType.getFolderName()).exists()) {
            serverDir = new File(selectedDir, serverType.getFolderName());
        }
        // Otherwise, assume the selected directory should contain the server
        else {
            serverDir = new File(selectedDir, serverType.getFolderName());
        }

        File runBat = new File(serverDir, "run.bat");
        boolean installed = serverDir.exists() && runBat.exists();

        // Return config with the parent directory as install path
        return new ServerConfig(serverType, serverDir.getParentFile(), installed);
    }

    public static void createMissingServerDirectories(ServerConfig config) {
        if (!isServerInstalled(config)) return;

        File serverDir = config.getServerDirectory();

        // Create any missing essential directories
        String[] directories = {
                "cg/sapp/lua",
                "cg/savegames",
                "maps",
                "sapp"
        };

        for (String dir : directories) {
            File directory = new File(serverDir, dir);
            if (!directory.exists()) {
                directory.mkdirs();
            }
        }
    }
}