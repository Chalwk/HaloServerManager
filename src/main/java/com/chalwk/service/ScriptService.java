package com.chalwk.service;

import com.chalwk.model.ScriptCategory;
import com.chalwk.model.ScriptMetadata;
import org.json.JSONObject;

import javax.swing.*;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileWriter;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

public class ScriptService {

    private static final String METADATA_URL = "https://raw.githubusercontent.com/Chalwk/HALO-SCRIPT-PROJECTS/master/metadata.json";
    private static final String BASE_SCRIPT_URL = "https://raw.githubusercontent.com/Chalwk/HALO-SCRIPT-PROJECTS/master/sapp/";

    public static List<ScriptMetadata> loadScriptsMetadata() {
        List<ScriptMetadata> scripts = new ArrayList<>();

        try {
            String jsonContent = fetchUrlContent(METADATA_URL);
            if (jsonContent != null) {
                JSONObject metadata = new JSONObject(jsonContent);
                parseCategoryScripts(metadata, "attractive", ScriptCategory.ATTRACTIVE, scripts);
                parseCategoryScripts(metadata, "custom_games", ScriptCategory.CUSTOM_GAMES, scripts);
                parseCategoryScripts(metadata, "utility", ScriptCategory.UTILITY, scripts);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return scripts;
    }

    private static void parseCategoryScripts(JSONObject metadata, String categoryKey,
                                             ScriptCategory category, List<ScriptMetadata> scripts) {
        if (metadata.has(categoryKey)) {
            JSONObject categoryObj = metadata.getJSONObject(categoryKey);
            for (String scriptKey : categoryObj.keySet()) {
                JSONObject scriptObj = categoryObj.getJSONObject(scriptKey);

                ScriptMetadata script = new ScriptMetadata();
                script.setKey(scriptKey);
                script.setCategory(category);

                // Handle different JSON structures
                if (scriptObj.has("title")) {
                    script.setTitle(scriptObj.getString("title"));
                } else if (scriptObj.has("truce")) {
                    script.setTitle(scriptObj.getString("truce"));
                }

                if (scriptObj.has("shortDescription")) {
                    script.setShortDescription(scriptObj.getString("shortDescription"));
                }

                if (scriptObj.has("description")) {
                    script.setDescription(scriptObj.getString("description"));
                }

                if (scriptObj.has("filename")) {
                    script.setFilename(scriptObj.getString("filename"));
                }

                scripts.add(script);
            }
        }
    }

    public static boolean downloadScript(ScriptMetadata script, File luaFolder, JProgressBar progressBar, JLabel statusLabel) {
        try {
            String scriptUrl = script.getRawScriptUrl();
            File outputFile = new File(luaFolder, script.getFilename());

            return downloadScriptFile(scriptUrl, outputFile, progressBar, statusLabel);

        } catch (Exception e) {
            SwingUtilities.invokeLater(() ->
                    statusLabel.setText("Error downloading script: " + e.getMessage()));
            return false;
        }
    }

    private static boolean downloadScriptFile(String scriptUrl, File outputFile,
                                              JProgressBar progressBar, JLabel statusLabel) {
        try {
            URL url = new URL(scriptUrl);
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");

            int responseCode = connection.getResponseCode();
            if (responseCode != HttpURLConnection.HTTP_OK) {
                SwingUtilities.invokeLater(() ->
                        statusLabel.setText("Download failed. Server returned HTTP code: " + responseCode));
                return false;
            }

            try (BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
                 FileWriter writer = new FileWriter(outputFile)) {

                String line;
                int lineCount = 0;

                while ((line = reader.readLine()) != null) {
                    writer.write(line + "\n");
                    lineCount++;

                    // Update progress every 10 lines
                    if (lineCount % 10 == 0) {
                        final int currentLines = lineCount;
                        SwingUtilities.invokeLater(() ->
                                statusLabel.setText("Downloaded " + currentLines + " lines..."));
                    }
                }
            }

            connection.disconnect();

            SwingUtilities.invokeLater(() -> {
                progressBar.setValue(100);
                statusLabel.setText("Script downloaded successfully!");
            });

            return true;

        } catch (Exception e) {
            SwingUtilities.invokeLater(() ->
                    statusLabel.setText("Download error: " + e.getMessage()));
            return false;
        }
    }

    private static String fetchUrlContent(String urlString) {
        try {
            URL url = new URL(urlString);
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");

            StringBuilder content = new StringBuilder();
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    content.append(line);
                }
            }

            connection.disconnect();
            return content.toString();

        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}