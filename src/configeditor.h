// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#ifndef CONFIGEDITOR_H
#define CONFIGEDITOR_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QMap>

class QSyntaxHighlighter;

class ConfigEditor : public QDialog
{
    Q_OBJECT
public:
    explicit ConfigEditor(const QString &serverPath, QWidget *parent = nullptr);

private slots:
    void onSave();
    void onFileChanged(int index);

private:
    void loadFile(const QString &absolutePath);
    void saveFile();
    void populateFiles();
    void setHighlighterForFile(const QString &filePath);

    QString m_serverPath;
    QComboBox *m_fileCombo;
    QTextEdit *m_textEdit;
    QPushButton *m_saveButton;
    QPushButton *m_closeButton;
    QString m_currentAbsolutePath;
    QMap<QString, QString> m_fileMap;
    QSyntaxHighlighter *m_highlighter;
};

#endif