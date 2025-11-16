/**
 * Halo Server Manager
 * Copyright (c) 2025 Jericho Crosby (Chalwk)
 * <p>
 * This project is licensed under the MIT License.
 * See LICENSE file for details:
 * https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
 */

package com.chalwk.model;

public enum ServerType {

    HPC("Halo PC", "HPC_Server", "https://github.com/Chalwk/HALO-SCRIPT-PROJECTS/releases/download/ReadyToGo/HPC_Server.zip"),
    HCE("Halo CE", "HCE_Server", "https://github.com/Chalwk/HALO-SCRIPT-PROJECTS/releases/download/ReadyToGo/HCE_Server.zip");

    private final String displayName;
    private final String folderName;
    private final String downloadUrl;

    ServerType(String displayName, String folderName, String downloadUrl) {
        this.displayName = displayName;
        this.folderName = folderName;
        this.downloadUrl = downloadUrl;
    }

    public String getDisplayName() {
        return displayName;
    }

    public String getFolderName() {
        return folderName;
    }

    public String getDownloadUrl() {
        return downloadUrl;
    }
}