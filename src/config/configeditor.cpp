// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "config/configeditor.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QSplitter>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <utility>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFile>
#include <QDir>

namespace
{
    class IniHighlighter : public QSyntaxHighlighter
    {
    public:
        IniHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent)
        {
            QTextCharFormat commentFormat;
            commentFormat.setForeground(Qt::darkGreen);
            commentFormat.setFontItalic(true);
            HighlightingRule rule;
            rule.pattern = QRegularExpression("^[;#].*");
            rule.format = commentFormat;
            highlightingRules.append(rule);

            QTextCharFormat sectionFormat;
            sectionFormat.setForeground(Qt::blue);
            sectionFormat.setFontWeight(QFont::Bold);
            rule.pattern = QRegularExpression("^\\[.*\\]");
            rule.format = sectionFormat;
            highlightingRules.append(rule);

            QTextCharFormat keyFormat;
            keyFormat.setForeground(Qt::darkBlue);
            keyFormat.setFontWeight(QFont::Bold);
            rule.pattern = QRegularExpression("^[^=]+(?==)");
            rule.format = keyFormat;
            highlightingRules.append(rule);

            QTextCharFormat valueFormat;
            valueFormat.setForeground(Qt::darkCyan);
            rule.pattern = QRegularExpression("(?<==).*$");
            rule.format = valueFormat;
            highlightingRules.append(rule);
        }

    protected:
        void highlightBlock(const QString &text) override
        {
            for (const HighlightingRule &rule : std::as_const(highlightingRules))
            {
                QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
                while (it.hasNext())
                {
                    QRegularExpressionMatch match = it.next();
                    setFormat(match.capturedStart(), match.capturedLength(), rule.format);
                }
            }
        }

    private:
        struct HighlightingRule
        {
            QRegularExpression pattern;
            QTextCharFormat format;
        };
        QVector<HighlightingRule> highlightingRules;
    };

    class LuaHighlighter : public QSyntaxHighlighter
    {
    public:
        LuaHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent)
        {
            QStringList keywordPatterns = {
                "and", "break", "do", "else", "elseif", "end", "false", "for",
                "function", "if", "in", "local", "nil", "not", "or", "repeat",
                "return", "then", "true", "until", "while"};

            QTextCharFormat keywordFormat;
            keywordFormat.setForeground(Qt::darkMagenta);
            keywordFormat.setFontWeight(QFont::Bold);

            for (const QString &pattern : keywordPatterns)
            {
                HighlightingRule rule;
                rule.pattern = QRegularExpression("\\b" + pattern + "\\b");
                rule.format = keywordFormat;
                highlightingRules.append(rule);
            }

            QTextCharFormat blockCommentFormat;
            blockCommentFormat.setForeground(Qt::darkGreen);
            blockCommentFormat.setFontItalic(true);
            HighlightingRule rule;
            rule.pattern = QRegularExpression("--[^\\[]*$");
            rule.format = blockCommentFormat;
            highlightingRules.append(rule);

            commentStartExpression = QRegularExpression("--\\[\\[");
            commentEndExpression = QRegularExpression("\\]\\]");
            commentFormat = blockCommentFormat;

            QTextCharFormat stringFormat;
            stringFormat.setForeground(Qt::darkRed);
            rule.pattern = QRegularExpression("\".*\"");
            rule.format = stringFormat;
            highlightingRules.append(rule);

            rule.pattern = QRegularExpression("'.*'");
            rule.format = stringFormat;
            highlightingRules.append(rule);

            QTextCharFormat numberFormat;
            numberFormat.setForeground(Qt::darkYellow);
            rule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?\\b");
            rule.format = numberFormat;
            highlightingRules.append(rule);
        }

    protected:
        void highlightBlock(const QString &text) override
        {
            for (const HighlightingRule &rule : std::as_const(highlightingRules))
            {
                QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
                while (it.hasNext())
                {
                    QRegularExpressionMatch match = it.next();
                    setFormat(match.capturedStart(), match.capturedLength(), rule.format);
                }
            }

            setCurrentBlockState(0);
            int startIndex = 0;
            if (previousBlockState() != 1)
                startIndex = text.indexOf(commentStartExpression);

            while (startIndex >= 0)
            {
                QRegularExpressionMatch matchEnd = commentEndExpression.match(text, startIndex);
                int endIndex = matchEnd.capturedStart();
                int commentLength;
                if (endIndex == -1)
                {
                    setCurrentBlockState(1);
                    commentLength = text.length() - startIndex;
                }
                else
                {
                    commentLength = endIndex - startIndex + matchEnd.capturedLength();
                }
                setFormat(startIndex, commentLength, commentFormat);
                startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
            }
        }

    private:
        struct HighlightingRule
        {
            QRegularExpression pattern;
            QTextCharFormat format;
        };
        QVector<HighlightingRule> highlightingRules;
        QTextCharFormat commentFormat;
        QRegularExpression commentStartExpression;
        QRegularExpression commentEndExpression;
    };

    class FileFilterModel : public QSortFilterProxyModel
    {
    public:
        explicit FileFilterModel(QObject *parent = nullptr)
            : QSortFilterProxyModel(parent)
        {
        }

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
        {
            auto *fs = qobject_cast<QFileSystemModel *>(sourceModel());
            if (!fs)
                return true;

            QModelIndex index = fs->index(sourceRow, 0, sourceParent);
            if (!index.isValid())
                return true;

            QFileInfo info = fs->fileInfo(index);

            if (info.isDir())
            {
                const QString relativePath =
                    QDir(fs->rootPath()).relativeFilePath(info.absoluteFilePath());

                const QString normalized =
                    QDir::fromNativeSeparators(relativePath).toLower();

                if (normalized == "maps" || normalized == "cg/savegames" || normalized == "cg/saved")
                    return false;
            }

            return true;
        }
    };

    class DragDropTreeView : public QTreeView
    {
    public:
        explicit DragDropTreeView(QWidget *parent = nullptr) : QTreeView(parent)
        {
            setAcceptDrops(true);
            setDragEnabled(true);
            setDropIndicatorShown(true);
            setDragDropMode(QAbstractItemView::DragDrop);
        }

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override
        {
            if (event->mimeData()->hasUrls())
                event->acceptProposedAction();
            else
                event->ignore();
        }

        void dragMoveEvent(QDragMoveEvent *event) override
        {
            if (event->mimeData()->hasUrls())
                event->acceptProposedAction();
            else
                event->ignore();
        }

        void dropEvent(QDropEvent *event) override
        {
            const QMimeData *mime = event->mimeData();
            if (!mime->hasUrls())
            {
                event->ignore();
                return;
            }

            QModelIndex index = indexAt(event->position().toPoint());
            QString targetDir;
            if (index.isValid())
            {
                auto *proxy = qobject_cast<QSortFilterProxyModel *>(model());
                QModelIndex sourceIndex = proxy ? proxy->mapToSource(index) : index;
                auto *fs = qobject_cast<QFileSystemModel *>(proxy ? proxy->sourceModel() : model());
                if (fs)
                {
                    QString filePath = fs->filePath(sourceIndex);
                    QFileInfo info(filePath);
                    if (info.isDir())
                        targetDir = filePath;
                    else
                        targetDir = info.absolutePath();
                }
            }
            if (targetDir.isEmpty())
            {
                auto *fs = qobject_cast<QFileSystemModel *>(
                    qobject_cast<QSortFilterProxyModel *>(model()) ? qobject_cast<QSortFilterProxyModel *>(model())->sourceModel() : model());
                if (fs)
                    targetDir = fs->rootPath();
            }
            if (targetDir.isEmpty())
            {
                event->ignore();
                return;
            }

            const QList<QUrl> urls = mime->urls();
            bool anyFailed = false;
            for (const QUrl &url : urls)
            {
                if (!url.isLocalFile())
                    continue;
                QString sourcePath = url.toLocalFile();
                QFileInfo srcInfo(sourcePath);
                QString destPath = QDir(targetDir).absoluteFilePath(srcInfo.fileName());

                if (srcInfo.isDir())
                {
                    if (!copyDirectoryRecursively(sourcePath, destPath))
                        anyFailed = true;
                }
                else
                {
                    if (!QFile::copy(sourcePath, destPath))
                        anyFailed = true;
                }
            }

            if (anyFailed)
            {
                QMessageBox::warning(this, "Copy Error",
                                     "Some files could not be copied. Check permissions and disk space.");
            }
            event->acceptProposedAction();
        }

    private:
        bool copyDirectoryRecursively(const QString &src, const QString &dst)
        {
            QDir srcDir(src);
            if (!srcDir.exists())
                return false;

            QDir dstDir(dst);
            if (!dstDir.exists() && !dstDir.mkpath("."))
                return false;

            bool success = true;
            for (const QString &entry : srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
            {
                QString srcPath = srcDir.absoluteFilePath(entry);
                QString dstPath = dstDir.absoluteFilePath(entry);
                QFileInfo info(srcPath);
                if (info.isDir())
                {
                    if (!copyDirectoryRecursively(srcPath, dstPath))
                        success = false;
                }
                else
                {
                    if (!QFile::copy(srcPath, dstPath))
                        success = false;
                }
            }
            return success;
        }
    };
}

ConfigEditor::ConfigEditor(const QString &serverPath, QWidget *parent)
    : QDialog(parent), m_serverPath(serverPath), m_highlighter(nullptr),
      m_encoding(Utf8), m_hasBom(false)
{
    setWindowTitle(QString("Server Files Editor - %1").arg(QDir::toNativeSeparators(m_serverPath)));
    resize(1200, 700);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setRootPath(m_serverPath);
    m_fileModel->setNameFilters({"*.txt", "*.cfg", "*.lua"});
    m_fileModel->setNameFilterDisables(false);
    m_fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_fileModel->setReadOnly(false);

    auto *proxyModel = new FileFilterModel(this);
    proxyModel->setSourceModel(m_fileModel);

    m_fileTree = new DragDropTreeView(this);
    m_fileTree->setModel(proxyModel);
    m_fileTree->setRootIndex(proxyModel->mapFromSource(m_fileModel->index(m_serverPath)));
    m_fileTree->setHeaderHidden(true);
    m_fileTree->setIndentation(15);
    m_fileTree->setMinimumWidth(300);

    m_fileTree->hideColumn(1);
    m_fileTree->hideColumn(2);
    m_fileTree->hideColumn(3);

    m_fileTree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

    connect(m_fileTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ConfigEditor::onFileSelected);

    splitter->addWidget(m_fileTree);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setFont(QFont("Courier New", 10));
    splitter->addWidget(m_textEdit);

    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("Save", this);
    m_closeButton = new QPushButton("Close", this);
    connect(m_saveButton, &QPushButton::clicked, this, &ConfigEditor::onSave);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addStretch();
    btnLayout->addWidget(m_saveButton);
    btnLayout->addWidget(m_closeButton);
    mainLayout->addLayout(btnLayout);

    m_fileTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_fileTree, &QTreeView::customContextMenuRequested,
            this, &ConfigEditor::onCustomContextMenu);

    m_contextMenu = new QMenu(this);
    m_newFileAction = new QAction("New File", this);
    m_newFolderAction = new QAction("New Folder", this);
    m_deleteAction = new QAction("Delete", this);

    connect(m_newFileAction, &QAction::triggered, this, &ConfigEditor::createNewFile);
    connect(m_newFolderAction, &QAction::triggered, this, &ConfigEditor::createNewFolder);
    connect(m_deleteAction, &QAction::triggered, this, &ConfigEditor::deleteSelected);

    m_contextMenu->addAction(m_newFileAction);
    m_contextMenu->addAction(m_newFolderAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_deleteAction);

    m_fileTree->expandToDepth(1);
}

QModelIndex ConfigEditor::getCurrentIndex() const
{
    QModelIndexList selected = m_fileTree->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return QModelIndex();
    return selected.first();
}

QString ConfigEditor::getCurrentDirectory() const
{
    QModelIndex idx = getCurrentIndex();
    if (!idx.isValid())
    {
        return m_serverPath;
    }

    auto *proxy = qobject_cast<QSortFilterProxyModel *>(m_fileTree->model());
    QModelIndex sourceIdx = proxy ? proxy->mapToSource(idx) : idx;
    QString path = m_fileModel->filePath(sourceIdx);
    QFileInfo info(path);
    return info.isDir() ? path : info.absolutePath();
}

QModelIndex ConfigEditor::mapToSource(const QModelIndex &index) const
{
    auto *proxy = qobject_cast<QSortFilterProxyModel *>(m_fileTree->model());
    return proxy ? proxy->mapToSource(index) : index;
}

void ConfigEditor::onCustomContextMenu(const QPoint &pos)
{
    QModelIndex idx = m_fileTree->indexAt(pos);
    if (idx.isValid())
    {
        m_fileTree->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
        m_deleteAction->setEnabled(true);
    }
    else
    {
        m_deleteAction->setEnabled(false);
    }
    m_contextMenu->exec(m_fileTree->viewport()->mapToGlobal(pos));
}

void ConfigEditor::createNewFile()
{
    QString dir = getCurrentDirectory();
    bool ok;
    QString name = QInputDialog::getText(this, "New File",
                                         "Enter file name (including extension):",
                                         QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty())
        return;

    QString fullPath = QDir(dir).absoluteFilePath(name);
    if (QFile::exists(fullPath))
    {
        QMessageBox::warning(this, "Error", "File already exists.");
        return;
    }

    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.close();
    }
    else
    {
        QMessageBox::warning(this, "Error", "Could not create file: " + file.errorString());
    }
}

void ConfigEditor::createNewFolder()
{
    QString dir = getCurrentDirectory();
    bool ok;
    QString name = QInputDialog::getText(this, "New Folder",
                                         "Enter folder name:",
                                         QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty())
        return;

    QString fullPath = QDir(dir).absoluteFilePath(name);
    if (QDir(fullPath).exists())
    {
        QMessageBox::warning(this, "Error", "Folder already exists.");
        return;
    }

    if (!QDir().mkdir(fullPath))
    {
        QMessageBox::warning(this, "Error", "Could not create folder.");
    }
}

void ConfigEditor::deleteSelected()
{
    QModelIndex idx = getCurrentIndex();
    if (!idx.isValid())
        return;

    auto *proxy = qobject_cast<QSortFilterProxyModel *>(m_fileTree->model());
    QModelIndex sourceIdx = proxy ? proxy->mapToSource(idx) : idx;
    QString path = m_fileModel->filePath(sourceIdx);
    QFileInfo info(path);

    if (QDir::toNativeSeparators(path) == QDir::toNativeSeparators(m_serverPath))
    {
        QMessageBox::warning(this, "Error", "Cannot delete the root directory.");
        return;
    }

    QString type = info.isDir() ? "folder" : "file";
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Delete",
        QString("Are you sure you want to delete the %1:\n%2?")
            .arg(type)
            .arg(QDir::toNativeSeparators(path)),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    bool success = false;
    if (info.isDir())
    {
        if (QDir(path).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).count() > 0)
        {
            QMessageBox::StandardButton recReply = QMessageBox::question(
                this,
                "Delete Non‑empty Folder",
                "The folder is not empty. Delete its contents as well?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (recReply == QMessageBox::Cancel)
                return;
            if (recReply == QMessageBox::Yes)
            {
                QDir dir(path);
                success = dir.removeRecursively();
            }
            else
            {
                QDir dir(path);
                if (dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty())
                    success = dir.rmdir(path);
                else
                    QMessageBox::warning(this, "Error", "Folder is not empty. Deletion aborted.");
            }
        }
        else
        {
            success = QDir(path).rmdir(path);
        }
    }
    else
    {
        success = QFile::remove(path);
    }

    if (!success)
        QMessageBox::warning(this, "Error", "Could not delete the selected item.");
}

void ConfigEditor::onFileSelected(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    auto *proxyModel = qobject_cast<QSortFilterProxyModel *>(m_fileTree->model());
    QModelIndex sourceIndex = proxyModel ? proxyModel->mapToSource(index) : index;

    QString absolutePath = m_fileModel->fileInfo(sourceIndex).absoluteFilePath();
    QFileInfo info(absolutePath);
    if (info.isFile())
        loadFile(absolutePath);
}

void ConfigEditor::loadFile(const QString &absolutePath)
{
    m_currentAbsolutePath = absolutePath;

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        m_textEdit->setPlainText("Cannot open file: " + file.errorString());
        setHighlighterForFile(QString());
        m_encoding = Utf8;
        m_hasBom = false;
        return;
    }

    QByteArray rawData = file.readAll();
    file.close();

    Encoding enc = Utf8;
    bool hasBom = false;
    QByteArray dataToDecode = rawData;

    if (rawData.size() >= 2)
    {
        const uchar *bytes = reinterpret_cast<const uchar *>(rawData.constData());
        if (bytes[0] == 0xFF && bytes[1] == 0xFE)
        {
            enc = Utf16LE;
            hasBom = true;
        }
        else if (bytes[0] == 0xFE && bytes[1] == 0xFF)
        {
            enc = Utf16BE;
            hasBom = true;
        }
        else if (rawData.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
        {
            enc = Utf8;
            hasBom = true;
            dataToDecode = rawData.mid(3);
        }
    }

    m_encoding = enc;
    m_hasBom = hasBom;

    QString text;
    switch (enc)
    {
    case Utf8:
    {
        QStringDecoder decoder(QStringDecoder::Utf8);
        text = decoder.decode(dataToDecode);
        if (decoder.hasError())
            text = QString::fromUtf8(dataToDecode);
        break;
    }
    case Utf16LE:
    {
        QStringDecoder decoder(QStringDecoder::Utf16LE);
        text = decoder.decode(rawData);
        if (decoder.hasError())
            text = QString::fromUtf16(reinterpret_cast<const char16_t *>(rawData.constData()), rawData.size() / 2);
        break;
    }
    case Utf16BE:
    {
        QStringDecoder decoder(QStringDecoder::Utf16BE);
        text = decoder.decode(rawData);
        if (decoder.hasError())
        {
            text = QString::fromUtf16(reinterpret_cast<const char16_t *>(rawData.constData()), rawData.size() / 2);
        }
        break;
    }
    }

    m_textEdit->setPlainText(text);
    setHighlighterForFile(absolutePath);
}

void ConfigEditor::setHighlighterForFile(const QString &filePath)
{
    if (m_highlighter)
    {
        delete m_highlighter;
        m_highlighter = nullptr;
    }

    if (filePath.isEmpty())
        return;

    QFileInfo info(filePath);
    QString fileName = info.fileName();
    QString suffix = info.suffix().toLower();

    if (fileName == "init.txt")
    {
        m_highlighter = new IniHighlighter(m_textEdit->document());
    }
    else if (suffix == "lua")
    {
        m_highlighter = new LuaHighlighter(m_textEdit->document());
    }
}

void ConfigEditor::saveFile()
{
    if (m_currentAbsolutePath.isEmpty())
    {
        QMessageBox::warning(this, "Error", "No file is currently loaded.");
        return;
    }

    QString text = m_textEdit->toPlainText();

    QByteArray encoded;
    switch (m_encoding)
    {
    case Utf8:
    {
        QStringEncoder encoder(QStringEncoder::Utf8);
        if (m_hasBom)
        {
            encoded = QByteArray("\xEF\xBB\xBF") + encoder.encode(text);
        }
        else
        {
            encoded = encoder.encode(text);
        }
        break;
    }
    case Utf16LE:
    {
        QStringEncoder encoder(QStringEncoder::Utf16LE,
                               m_hasBom ? QStringEncoder::Flag::WriteBom : QStringEncoder::Flag::Stateless);
        encoded = encoder.encode(text);
        break;
    }
    case Utf16BE:
    {
        QStringEncoder encoder(QStringEncoder::Utf16BE,
                               m_hasBom ? QStringEncoder::Flag::WriteBom : QStringEncoder::Flag::Stateless);
        encoded = encoder.encode(text);
        break;
    }
    }

    QFile file(m_currentAbsolutePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QMessageBox::warning(this, "Error", "Could not save file: " + file.errorString());
        return;
    }

    file.write(encoded);
    file.close();

    QMessageBox::information(this, "Saved", "File saved successfully.");
}

void ConfigEditor::onSave()
{
    saveFile();
}