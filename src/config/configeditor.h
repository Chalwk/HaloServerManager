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
#include <QTreeView>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QAction>

class QSyntaxHighlighter;

class ConfigEditor : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigEditor(const QString &serverPath, QWidget *parent = nullptr);

private slots:
    void onSave();
    void onFileSelected(const QModelIndex &index);
    void onCustomContextMenu(const QPoint &pos);
    void createNewFile();
    void createNewFolder();
    void deleteSelected();

private:
    enum Encoding
    {
        Utf8,
        Utf16LE,
        Utf16BE
    };

    void loadFile(const QString &absolutePath);
    void saveFile();
    void setHighlighterForFile(const QString &filePath);
    QModelIndex getCurrentIndex() const;
    QString getCurrentDirectory() const;
    QModelIndex mapToSource(const QModelIndex &index) const;

    QString m_serverPath;
    QTreeView *m_fileTree;
    QFileSystemModel *m_fileModel;
    QTextEdit *m_textEdit;
    QPushButton *m_saveButton;
    QPushButton *m_closeButton;
    QString m_currentAbsolutePath;
    QSyntaxHighlighter *m_highlighter;

    Encoding m_encoding;
    bool m_hasBom;

    QMenu *m_contextMenu;
    QAction *m_newFileAction;
    QAction *m_newFolderAction;
    QAction *m_deleteAction;
};

#endif