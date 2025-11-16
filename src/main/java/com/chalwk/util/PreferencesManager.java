/**
 * Halo Server Manager
 * Copyright (c) 2025 Jericho Crosby (Chalwk)
 * <p>
 * This project is licensed under the MIT License.
 * See LICENSE file for details:
 * https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
 */

package com.chalwk.util;

import java.io.*;
import java.util.Properties;

public class PreferencesManager {
    private static final String CONFIG_FILE = "halo_server_manager.properties";
    private final Properties properties;

    public PreferencesManager() {
        properties = new Properties();
        load();
    }

    public void setInstallationPath(String serverType, String path) {
        properties.setProperty(serverType + ".install.path", path);
        save();
    }

    public String getInstallationPath(String serverType) {
        return properties.getProperty(serverType + ".install.path");
    }

    private void load() {
        try (InputStream input = new FileInputStream(CONFIG_FILE)) {
            properties.load(input);
        } catch (IOException e) {
            // Config file doesn't exist, use defaults
        }
    }

    private void save() {
        try (OutputStream output = new FileOutputStream(CONFIG_FILE)) {
            properties.store(output, "Halo Server Manager Configuration");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void setUpdatePreference(String key, String value) {
        properties.setProperty("update." + key, value);
        save();
    }

    public String getUpdatePreference(String key, String defaultValue) {
        return properties.getProperty("update." + key, defaultValue);
    }

    public boolean getAutoUpdateEnabled() {
        return Boolean.parseBoolean(getUpdatePreference("autoCheck", "true"));
    }

    public void setAutoUpdateEnabled(boolean enabled) {
        setUpdatePreference("autoCheck", String.valueOf(enabled));
    }

    public String getSkippedVersion() {
        return getUpdatePreference("skippedVersion", "");
    }

    public void setSkippedVersion(String version) {
        setUpdatePreference("skippedVersion", version);
    }
}