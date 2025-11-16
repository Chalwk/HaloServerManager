package com.chalwk.model;

import java.io.File;

public class ServerConfig {
    private ServerType serverType;
    private File installPath;
    private boolean installed;

    public ServerConfig(ServerType serverType, File installPath, boolean installed) {
        this.serverType = serverType;
        this.installPath = installPath;
        this.installed = installed;
    }

    // Getters and setters
    public ServerType getServerType() {
        return serverType;
    }

    public void setServerType(ServerType serverType) {
        this.serverType = serverType;
    }

    public File getInstallPath() {
        return installPath;
    }

    public void setInstallPath(File installPath) {
        this.installPath = installPath;
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