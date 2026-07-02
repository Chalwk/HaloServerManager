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
#include <utility>

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

            QTextCharFormat commentFormat;
            commentFormat.setForeground(Qt::darkGreen);
            commentFormat.setFontItalic(true);
            HighlightingRule rule;
            rule.pattern = QRegularExpression("--[^\\[]*$");
            rule.format = commentFormat;
            highlightingRules.append(rule);

            commentStartExpression = QRegularExpression("--\\[\\[");
            commentEndExpression = QRegularExpression("\\]\\]");

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
    : QDialog(parent), m_serverPath(serverPath), m_highlighter(nullptr)
{
    setWindowTitle("Server Files Editor");
    resize(1100, 650);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("File:"));

    m_fileCombo = new QComboBox();
    populateFiles();
    connect(m_fileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigEditor::onFileChanged);
    topLayout->addWidget(m_fileCombo);
    topLayout->addStretch();
    layout->addLayout(topLayout);

    m_textEdit = new QTextEdit();
    m_textEdit->setFont(QFont("Courier New", 10));
    layout->addWidget(m_textEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("Save");
    m_closeButton = new QPushButton("Close");
    connect(m_saveButton, &QPushButton::clicked, this, &ConfigEditor::onSave);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addStretch();
    btnLayout->addWidget(m_saveButton);
    btnLayout->addWidget(m_closeButton);
    layout->addLayout(btnLayout);

    if (m_fileCombo->count() > 0)
    {
        int initIndex = m_fileCombo->findText("cg/init.txt");
        if (initIndex == -1)
            initIndex = 0;
        m_fileCombo->setCurrentIndex(initIndex);
        onFileChanged(initIndex);
    }
}

void ConfigEditor::populateFiles()
{
    m_fileMap.clear();
    m_fileCombo->clear();

    QStringList dirs = {".", "cg", "sapp", "cg/sapp", "cg/sapp/lua"};
    QStringList extensions = {"*.txt", "*.cfg", "*.lua"};

    for (const QString &dir : dirs)
    {
        QDir scanDir(QDir(m_serverPath).absoluteFilePath(dir));
        if (!scanDir.exists())
            continue;

        QStringList files;
        for (const QString &ext : extensions)
            files << scanDir.entryList({ext}, QDir::Files);

        for (const QString &file : files)
        {
            QString relativePath = QDir(dir).filePath(file);
            relativePath = QDir::cleanPath(relativePath);
            QString absolutePath = scanDir.absoluteFilePath(file);
            if (!m_fileMap.contains(relativePath))
            {
                m_fileMap[relativePath] = absolutePath;
                m_fileCombo->addItem(relativePath);
            }
        }
    }

    m_fileCombo->model()->sort(0);

    if (m_fileCombo->count() == 0)
    {
        m_fileCombo->addItem("No editable files found");
        m_fileCombo->setEnabled(false);
    }
    else
    {
        m_fileCombo->setEnabled(true);
    }
}

void ConfigEditor::onFileChanged(int index)
{
    if (index < 0 || index >= m_fileCombo->count())
        return;
    QString display = m_fileCombo->itemText(index);
    if (display == "No editable files found")
        return;

    QString absolutePath = m_fileMap.value(display);
    if (!absolutePath.isEmpty())
        loadFile(absolutePath);
}

void ConfigEditor::loadFile(const QString &absolutePath)
{
    m_currentAbsolutePath = absolutePath;

    QFile file(absolutePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        m_textEdit->setPlainText(stream.readAll());
        file.close();
        setHighlighterForFile(absolutePath);
    }
    else
    {
        m_textEdit->setPlainText("Cannot open file: " + file.errorString());
        setHighlighterForFile(QString());
    }
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

    QFile file(m_currentAbsolutePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QTextStream stream(&file);
        stream << m_textEdit->toPlainText();
        file.close();
        QMessageBox::information(this, "Saved", "File saved successfully.");
    }
    else
    {
        QMessageBox::warning(this, "Error", "Could not save file: " + file.errorString());
    }
}

void ConfigEditor::onSave()
{
    saveFile();
}