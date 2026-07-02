#include "serverprocess.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winternl.h>

static bool getWindowsVersion(DWORD &major, DWORD &minor, DWORD &build)
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll)
        return false;
    typedef LONG(WINAPI * RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
    RtlGetVersion_t RtlGetVersion = (RtlGetVersion_t)GetProcAddress(hNtdll, "RtlGetVersion");
    if (!RtlGetVersion)
        return false;
    RTL_OSVERSIONINFOW ver = {sizeof(RTL_OSVERSIONINFOW)};
    if (RtlGetVersion(&ver) != 0)
        return false;
    major = ver.dwMajorVersion;
    minor = ver.dwMinorVersion;
    build = ver.dwBuildNumber;
    return true;
}

static void readPipe(HANDLE hPipe, QByteArray &buffer,
                     std::function<void(const QString &, bool)> callback,
                     bool isError)
{
    DWORD avail = 0;
    if (!PeekNamedPipe(hPipe, nullptr, 0, nullptr, &avail, nullptr) || avail == 0)
        return;
    char temp[4096];
    DWORD read = 0;
    if (!ReadFile(hPipe, temp, sizeof(temp), &read, nullptr) || read == 0)
        return;
    buffer.append(temp, read);
    int pos;
    while ((pos = buffer.indexOf('\n')) != -1)
    {
        QByteArray line = buffer.left(pos);
        buffer.remove(0, pos + 1);
        if (line.endsWith('\r'))
            line.chop(1);
        if (!line.isEmpty())
            callback(QString::fromUtf8(line), isError);
    }
}
#endif

ServerProcess::ServerProcess(const QString &serverPath, const QString &serverType, int port, QObject *parent)
    : QObject(parent), m_serverPath(serverPath), m_serverType(serverType), m_port(port), m_autoRestart(false), m_restartDelay(5), m_restarting(false) // NEW
#ifdef Q_OS_WIN
      ,
      m_hProcess(nullptr), m_pid(0), m_console(nullptr), m_outputPipe(nullptr), m_inputPipe(nullptr), m_outputNotifier(nullptr), m_processNotifier(nullptr), pCreatePseudoConsole(nullptr), pClosePseudoConsole(nullptr)
#endif
{
    m_restartTimer = new QTimer(this);
    m_restartTimer->setSingleShot(true);
    connect(m_restartTimer, &QTimer::timeout, this, &ServerProcess::onAutoRestart);
}

ServerProcess::~ServerProcess()
{
    stop();
    CleanupPseudoConsoleAndPipes();
}

#ifdef Q_OS_WIN
bool ServerProcess::initConPtyFunctions()
{
    HMODULE hModule = GetModuleHandleW(L"kernel32.dll");
    if (!hModule)
        hModule = GetModuleHandleW(L"kernelbase.dll");
    if (!hModule)
        return false;

    pCreatePseudoConsole = (CreatePseudoConsole_t)GetProcAddress(hModule, "CreatePseudoConsole");
    pClosePseudoConsole = (ClosePseudoConsole_t)GetProcAddress(hModule, "ClosePseudoConsole");

    if (!pCreatePseudoConsole)
        qWarning() << "CreatePseudoConsole not found";
    if (!pClosePseudoConsole)
        qWarning() << "ClosePseudoConsole not found";

    return (pCreatePseudoConsole && pClosePseudoConsole);
}
#endif

bool ServerProcess::start()
{
    if (isRunning())
        return false;

#ifdef Q_OS_WIN
    DWORD major, minor, build;
    if (getWindowsVersion(major, minor, build))
    {
        qDebug() << "Windows version:" << major << "." << minor << "build" << build;
        if (build < 17763)
        {
            QString msg = QString("ConPTY requires Windows 10 build 17763 or later.\n"
                                  "Your build is %1.\n"
                                  "Please upgrade Windows or use the batch file manually.")
                              .arg(build);
            QMessageBox::warning(nullptr, "Unsupported Windows", msg);
            qWarning() << msg;
            return false;
        }
    }

    if (!initConPtyFunctions())
    {
        QString msg = "ConPTY functions not available.\n"
                      "Please ensure you have Windows 10 version 1809 (build 17763) or later.\n"
                      "Also check that your Windows is up-to-date.";
        QMessageBox::warning(nullptr, "Missing ConPTY", msg);
        qWarning() << msg;
        return false;
    }

    QString exeName;
    if (m_serverType == "SAPP_CE")
        exeName = "haloceded.exe";
    else if (m_serverType == "SAPP_PC")
        exeName = "haloded.exe";
    else
        return false;

    QString executable = QDir(m_serverPath).absoluteFilePath(exeName);
    if (!QFileInfo::exists(executable))
    {
        qWarning() << "Executable not found:" << executable;
        return false;
    }

    QString cgPath = QDir(m_serverPath).absoluteFilePath("cg");
    QString execPath = QDir(cgPath).absoluteFilePath("init.txt");

    QStringList args;
    args << "-path" << QDir::toNativeSeparators(cgPath)
         << "-exec" << QDir::toNativeSeparators(execPath)
         << "-port" << QString::number(m_port);

    QString cmdLine = "\"" + executable + "\"";
    for (const QString &arg : args)
    {
        cmdLine += " \"" + arg + "\"";
    }

    HANDLE hOutPipeRead, hOutPipeWrite;
    HANDLE hInPipeRead, hInPipeWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

    if (!CreatePipe(&hOutPipeRead, &hOutPipeWrite, &sa, 0) ||
        !CreatePipe(&hInPipeRead, &hInPipeWrite, &sa, 0))
    {
        qWarning() << "CreatePipe failed";
        return false;
    }

    COORD size = {80, 40};
    HRESULT hr = pCreatePseudoConsole(size, hInPipeRead, hOutPipeWrite, 0, &m_console);
    if (FAILED(hr))
    {
        qWarning() << "CreatePseudoConsole failed:" << hr;
        CloseHandle(hOutPipeRead);
        CloseHandle(hOutPipeWrite);
        CloseHandle(hInPipeRead);
        CloseHandle(hInPipeWrite);
        return false;
    }

    m_outputPipe = hOutPipeRead;
    m_inputPipe = hInPipeWrite;

    CloseHandle(hOutPipeWrite);
    CloseHandle(hInPipeRead);

    STARTUPINFOEX si = {0};
    si.StartupInfo.cb = sizeof(si);

    SIZE_T attrSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);
    char attrBuffer[256];
    LPPROC_THREAD_ATTRIBUTE_LIST pAttrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrBuffer);
    if (!InitializeProcThreadAttributeList(pAttrList, 1, 0, &attrSize))
    {
        qWarning() << "InitializeProcThreadAttributeList failed";
        CleanupPseudoConsoleAndPipes();
        return false;
    }
    if (!UpdateProcThreadAttribute(pAttrList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   m_console, sizeof(HPCON), nullptr, nullptr))
    {
        qWarning() << "UpdateProcThreadAttribute failed";
        DeleteProcThreadAttributeList(pAttrList);
        CleanupPseudoConsoleAndPipes();
        return false;
    }
    si.lpAttributeList = pAttrList;

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcess(
        nullptr,
        (LPWSTR)cmdLine.utf16(),
        nullptr,
        nullptr,
        TRUE,
        EXTENDED_STARTUPINFO_PRESENT,
        nullptr,
        (LPCWSTR)m_serverPath.utf16(),
        &si.StartupInfo,
        &pi);

    DeleteProcThreadAttributeList(pAttrList);

    if (!ok)
    {
        qWarning() << "CreateProcess failed:" << GetLastError();
        CleanupPseudoConsoleAndPipes();
        return false;
    }

    CloseHandle(pi.hThread);
    m_hProcess = pi.hProcess;
    m_pid = pi.dwProcessId;

    m_processNotifier = new QWinEventNotifier(m_hProcess, this);
    connect(m_processNotifier, &QWinEventNotifier::activated, this, &ServerProcess::onProcessExited);
    m_processNotifier->setEnabled(true);

    m_outputNotifier = new QWinEventNotifier(m_outputPipe, this);
    connect(m_outputNotifier, &QWinEventNotifier::activated, this, &ServerProcess::onPipeReady);
    m_outputNotifier->setEnabled(true);

    m_startTime = QDateTime::currentDateTime();
    emit stateChanged(true);
    return true;

#else
    qWarning() << "ConPTY only works on Windows.";
    return false;
#endif
}

void ServerProcess::stop()
{
    if (m_hProcess)
    {
        TerminateProcess(m_hProcess, 0);
        WaitForSingleObject(m_hProcess, 3000);
    }
    CleanupPseudoConsoleAndPipes();
}

void ServerProcess::restart()
{
    m_restarting = true;
    stop();
    start();
    m_restarting = false;
}

bool ServerProcess::isRunning() const
{
    if (!m_hProcess)
        return false;
    DWORD exitCode;
    if (GetExitCodeProcess(m_hProcess, &exitCode))
    {
        return (exitCode == STILL_ACTIVE);
    }
    return false;
}

void ServerProcess::sendCommand(const QString &cmd)
{
    if (!m_inputPipe || !isRunning())
        return;
    QByteArray data = (cmd + "\n").toUtf8();
    DWORD written;
    WriteFile(m_inputPipe, data.constData(), data.size(), &written, nullptr);
}

qint64 ServerProcess::uptime() const
{
    if (!isRunning())
        return 0;
    return m_startTime.secsTo(QDateTime::currentDateTime());
}

void ServerProcess::setAutoRestart(bool enabled, int delaySeconds)
{
    m_autoRestart = enabled;
    m_restartDelay = delaySeconds;
}

void ServerProcess::onProcessExited()
{
    DWORD exitCode = 0;
    if (m_hProcess)
        GetExitCodeProcess(m_hProcess, &exitCode);

    m_startTime = QDateTime();
    bool crashed = (exitCode != 0);

    if (crashed)
    {
        emit processCrashed();
        if (m_autoRestart)
        {
            m_restartTimer->start(m_restartDelay * 1000);
        }
    }

    emit stateChanged(false);
    CleanupPseudoConsoleAndPipes();
}

void ServerProcess::onPipeReady()
{
    if (!m_outputPipe)
        return;
    readPipe(m_outputPipe, m_stdoutBuffer, [this](const QString &line, bool)
             { emit logLine(line, false); }, false);
}

void ServerProcess::onAutoRestart()
{
    start();
}

void ServerProcess::CleanupPseudoConsoleAndPipes()
{
#ifdef Q_OS_WIN
    if (m_processNotifier)
    {
        m_processNotifier->setEnabled(false);
        delete m_processNotifier;
        m_processNotifier = nullptr;
    }
    if (m_outputNotifier)
    {
        m_outputNotifier->setEnabled(false);
        delete m_outputNotifier;
        m_outputNotifier = nullptr;
    }
    if (m_console && pClosePseudoConsole)
    {
        pClosePseudoConsole(m_console);
        m_console = nullptr;
    }
    if (m_outputPipe)
    {
        CloseHandle(m_outputPipe);
        m_outputPipe = nullptr;
    }
    if (m_inputPipe)
    {
        CloseHandle(m_inputPipe);
        m_inputPipe = nullptr;
    }
    if (m_hProcess)
    {
        CloseHandle(m_hProcess);
        m_hProcess = nullptr;
    }
    m_pid = 0;
#endif
}