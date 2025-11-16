/**
 * Halo Server Manager
 * Copyright (c) 2025 Jericho Crosby (Chalwk)
 * <p>
 * This project is licensed under the MIT License.
 * See LICENSE file for details:
 * https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
 */

package com.chalwk.model;

public enum ScriptCategory {
    ATTRACTIVE("Attractive", "attractive"),
    CUSTOM_GAMES("Custom Games", "custom_games"),
    UTILITY("Utility", "utility");

    private final String displayName;
    private final String folderName;

    ScriptCategory(String displayName, String folderName) {
        this.displayName = displayName;
        this.folderName = folderName;
    }

    public String getDisplayName() {
        return displayName;
    }

    public String getFolderName() {
        return folderName;
    }
}