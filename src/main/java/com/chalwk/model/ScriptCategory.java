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

    public String getDisplayName() { return displayName; }
    public String getFolderName() { return folderName; }
}