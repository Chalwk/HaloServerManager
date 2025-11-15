package com.chalwk.util;

import java.io.*;
import java.util.Properties;

public class PreferencesManager {
    private static final String CONFIG_FILE = "halo_server_manager.properties";
    private Properties properties;

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
}