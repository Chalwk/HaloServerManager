// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "configeditor.h"
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
#include <utility>
#include <QStringDecoder>
#include <QStringEncoder>

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
}

ConfigEditor::ConfigEditor(const QString &serverPath, QWidget *parent)
    : QDialog(parent), m_serverPath(serverPath), m_highlighter(nullptr),
      m_encoding(Utf8), m_hasBom(false)
{
    setWindowTitle("Server Files Editor");
    resize(1200, 700);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setRootPath(m_serverPath);
    m_fileModel->setNameFilters({"*.txt", "*.cfg", "*.lua"});
    m_fileModel->setNameFilterDisables(false);
    m_fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    m_fileTree = new QTreeView(this);
    m_fileTree->setModel(m_fileModel);
    m_fileTree->setRootIndex(m_fileModel->index(m_serverPath));
    m_fileTree->setHeaderHidden(true);
    m_fileTree->setIndentation(15);
    m_fileTree->setMinimumWidth(300);

    m_fileTree->hideColumn(1); // Size
    m_fileTree->hideColumn(2); // Type
    m_fileTree->hideColumn(3); // Date modified

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

    m_fileTree->expandToDepth(1);
}

void ConfigEditor::onFileSelected(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString absolutePath = m_fileModel->fileInfo(index).absoluteFilePath();
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