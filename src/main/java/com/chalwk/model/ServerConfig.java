/**
 * Halo Server Manager
 * Copyright (c) 2025 Jericho Crosby (Chalwk)
 * <p>
 * This project is licensed under the MIT License.
 * See LICENSE file for details:
 * https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
 */

package com.chalwk.model;

import java.io.File;

public class ServerConfig {
    private final ServerType serverType;
    private final File installPath;
    private boolean installed;

    public ServerConfig(ServerType serverType, File installPath, boolean installed) {
        this.serverType = serverType;
        this.installPath = installPath;
        this.installed = installed;
    }

    // Getters and setters
    public File getInstallPath() {
        return installPath;
    }

    public boolean isInstalled() {
        return installed;
    }

    public void setInstalled(boolean installed) {
        this.installed = installed;
    }

    public File getServerDirectory() {
        return new File(installPath, serverType.getFolderName());
    }

    public File getRunBatFile() {
        return new File(getServerDirectory(), "run.bat");
    }
}