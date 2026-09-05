/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UiAccessInjector.h"

#include "config-keepassx.h"
#include "core/Tools.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUuid>

#include <cstring>

#include <sddl.h>
#include <shellapi.h>
#include <shlobj.h>

namespace
{
    // A constant, not #ifdef: compiling the body out would leave unused static functions (-Werror).
#ifdef KPXC_FEATURE_UIACCESS_HELPER
    constexpr bool s_featureEnabled = true;
#else
    constexpr bool s_featureEnabled = false;
#endif

    const char* s_credentialDialogClass = "Credential Dialog Xaml Host";
    const char* s_brokerImage = "CredentialUIBroker.exe";

    /* One ACE for this user's SID, protected against inherited ACEs. The pipe name
     * travels on the command line, so the binding rests on the identity checks at
     * both ends, not on the name. */
    QString currentUserDacl()
    {
        HANDLE token = nullptr;
        if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)) {
            return {};
        }
        DWORD size = 0;
        ::GetTokenInformation(token, TokenUser, nullptr, 0, &size);
        QByteArray buffer(static_cast<int>(size), 0);
        QString sddl;
        if (::GetTokenInformation(token, TokenUser, buffer.data(), size, &size)) {
            LPWSTR sid = nullptr;
            const auto* user = reinterpret_cast<const TOKEN_USER*>(buffer.constData());
            if (::ConvertSidToStringSidW(user->User.Sid, &sid)) {
                sddl = QStringLiteral("D:P(A;;GA;;;%1)").arg(QString::fromWCharArray(sid));
                ::LocalFree(sid);
            }
        }
        ::CloseHandle(token);
        return sddl;
    }

    /* The directories Windows grants uiAccess from; a standard user cannot write to them. */
    bool inUiAccessLocation(const QString& directory)
    {
        // From the OS, not the environment: %ProgramFiles% is settable by the launcher.
        QStringList roots;
        for (const auto& id : {FOLDERID_ProgramFilesX64, FOLDERID_ProgramFilesX86, FOLDERID_ProgramFiles}) {
            PWSTR folder = nullptr;
            if (SUCCEEDED(::SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &folder))) {
                roots << QString::fromWCharArray(folder);
                ::CoTaskMemFree(folder);
            }
        }
        // Not GetSystemDirectoryW: redirected to SysWOW64 for a 32-bit process.
        wchar_t windows[MAX_PATH] = {0};
        const UINT length = ::GetWindowsDirectoryW(windows, MAX_PATH);
        if (length > 0 && length < MAX_PATH) {
            roots << QDir(QString::fromWCharArray(windows)).filePath(QStringLiteral("System32"));
        }
        // Canonical first: cleanPath does not expand an 8.3 alias.
        const auto resolved = QFileInfo(directory).canonicalFilePath();
        const auto path = QDir::cleanPath(resolved.isEmpty() ? directory : resolved) + QLatin1Char('/');
        for (const auto& root : roots) {
            if (root.isEmpty()) {
                continue;
            }
            const auto prefix = QDir::cleanPath(root) + QLatin1Char('/');
            if (path.startsWith(prefix, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    QString processImagePath(DWORD pid)
    {
        HANDLE process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process) {
            return {};
        }
        wchar_t path[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        QString image;
        if (::QueryFullProcessImageNameW(process, 0, path, &size)) {
            image = QDir::fromNativeSeparators(QString::fromWCharArray(path, size));
        }
        ::CloseHandle(process);
        return image;
    }

    /** The full path the real credential broker runs from, or empty. */
    QString brokerImagePath()
    {
        wchar_t windows[MAX_PATH] = {0};
        const UINT length = ::GetWindowsDirectoryW(windows, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) {
            return {};
        }
        return QDir(QString::fromWCharArray(windows))
            .filePath(QStringLiteral("System32/%1").arg(QLatin1String(s_brokerImage)));
    }
} // namespace

UiAccessInjector::~UiAccessInjector()
{
    end();
}

QString UiAccessInjector::helperPath()
{
    if (!s_featureEnabled) {
        return {};
    }
    // Beside the application only; a second location could yield a writable copy.
    const auto helper =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("keepassxc-uiaccess-helper.exe"));
    if (!QFileInfo::exists(helper)) {
        return {};
    }
    // A writable install (portable ZIP, build tree) cannot hold a trusted helper.
    if (!inUiAccessLocation(QCoreApplication::applicationDirPath())) {
        qWarning("Auto-Type: not delegating to a uiAccess helper outside a protected location");
        return {};
    }
    return QDir::toNativeSeparators(helper);
}

bool UiAccessInjector::isBrokeredCredentialDialog(HWND window)
{
    if (!window) {
        return false;
    }
    wchar_t className[256] = {0};
    if (::GetClassNameW(window, className, 255) <= 0) {
        return false;
    }
    if (QString::fromWCharArray(className) != QLatin1String(s_credentialDialogClass)) {
        return false;
    }
    // The class alone matches a look-alike in any process: the owner must be the
    // broker in System32. The helper repeats this check.
    DWORD pid = 0;
    ::GetWindowThreadProcessId(window, &pid);
    const auto expected = brokerImagePath();
    if (expected.isEmpty()) {
        return false;
    }
    return processImagePath(pid).compare(expected, Qt::CaseInsensitive) == 0;
}

bool UiAccessInjector::begin(HWND target)
{
    m_helperFailed = false;
    m_interrupted = false;
    if (active() && m_target == target) {
        return true;
    }
    end();

    const auto helper = helperPath();
    if (helper.isEmpty()) {
        return false;
    }

    const auto name = QStringLiteral("keepassxc-uiaccess-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    const auto pipeName = QStringLiteral("\\\\.\\pipe\\%1").arg(name);

    const auto sddl = currentUserDacl();
    if (sddl.isEmpty()) {
        return false;
    }
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(
            reinterpret_cast<LPCWSTR>(sddl.utf16()), SDDL_REVISION_1, &descriptor, nullptr)) {
        return false;
    }
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = descriptor;

    // FIRST_PIPE_INSTANCE: fail rather than join an existing object. One instance.
    // Duplex: the helper answers each batch with one byte.
    m_pipe = ::CreateNamedPipeW(reinterpret_cast<LPCWSTR>(pipeName.utf16()),
                                PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                1,
                                sizeof(DWORD) + 64 * sizeof(INPUT),
                                16,
                                5000,
                                &attributes);
    ::LocalFree(descriptor);
    if (m_pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    // ShellExecuteEx: the uiAccess grant comes from AppInfo, which CreateProcess
    // does not consult (ERROR_ELEVATION_REQUIRED). Nothing is inherited.
    const auto arguments = QStringLiteral("--pipe %1 --target %2").arg(name).arg(reinterpret_cast<quintptr>(target));

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    info.lpVerb = L"open";
    info.lpFile = reinterpret_cast<LPCWSTR>(helper.utf16());
    info.lpParameters = reinterpret_cast<LPCWSTR>(arguments.utf16());
    // Must not take the foreground from the target.
    info.nShow = SW_HIDE;

    // Success with a null hProcess leaves no pid to admit and no exit code to read.
    if (!::ShellExecuteExW(&info) || !info.hProcess) {
        end();
        return false;
    }
    m_process = info.hProcess;

    if (!waitForHelper()) {
        end();
        return false;
    }

    m_target = target;
    return true;
}

bool UiAccessInjector::waitForHelper()
{
    // Non-blocking so the wait has a deadline: ConnectNamedPipe returns
    // ERROR_PIPE_LISTENING until a client arrives.
    DWORD mode = PIPE_TYPE_BYTE | PIPE_NOWAIT;
    if (!::SetNamedPipeHandleState(m_pipe, &mode, nullptr, nullptr)) {
        return false;
    }

    const DWORD helperPid = m_process ? ::GetProcessId(m_process) : 0;
    bool connected = false;
    // Wall clock, not iterations: a client reconnecting in a loop must not spend the budget.
    const ULONGLONG deadline = ::GetTickCount64() + s_helperConnectTimeoutMs;
    while (::GetTickCount64() < deadline) {
        if (::ConnectNamedPipe(m_pipe, nullptr) || ::GetLastError() == ERROR_PIPE_CONNECTED) {
            // Only the process that was started: the name is readable from its command line.
            DWORD clientPid = 0;
            if (::GetNamedPipeClientProcessId(m_pipe, &clientPid) && clientPid == helperPid) {
                connected = true;
                break;
            }
            // Dropped, not fatal: after the disconnect the pipe listens again
            // (ConnectNamedPipe reports ERROR_PIPE_LISTENING until the next client).
            ::DisconnectNamedPipe(m_pipe);
            Tools::wait(s_helperConnectPollMs);
            continue;
        }
        if (::GetLastError() != ERROR_PIPE_LISTENING) {
            return false;
        }
        // A helper that exited has refused (no uiAccess).
        if (m_process && ::WaitForSingleObject(m_process, 0) == WAIT_OBJECT_0) {
            return false;
        }
        // Pumped: this runs on the GUI thread.
        Tools::wait(s_helperConnectPollMs);
    }
    if (!connected) {
        return false;
    }

    // Left non-blocking: every wait below is bounded and pumped.
    return true;
}

bool UiAccessInjector::active() const
{
    return m_pipe != INVALID_HANDLE_VALUE && m_target;
}

bool UiAccessInjector::send(const INPUT* inputs, int count)
{
    if (!active() || count <= 0 || count > 64) {
        return false;
    }
    const DWORD records = static_cast<DWORD>(count);
    const DWORD bytes = records * static_cast<DWORD>(sizeof(INPUT));

    // One write: a header sent alone could leave the helper waiting on a body.
    char message[sizeof(DWORD) + 64 * sizeof(INPUT)];
    ::memcpy(message, &records, sizeof(records));
    ::memcpy(message + sizeof(records), inputs, bytes);
    const DWORD size = static_cast<DWORD>(sizeof(records) + bytes);

    // NOWAIT byte mode: a full buffer returns TRUE with fewer bytes written (0 or
    // partial); a closed client fails with ERROR_NO_DATA. Resume what did not
    // finish -- the helper reads whole messages -- and treat any failure as
    // final. Bounded by s_sendRetryMs.
    DWORD offset = 0;
    bool sent = false;
    const ULONGLONG deadline = ::GetTickCount64() + s_sendRetryMs;
    for (;;) {
        DWORD written = 0;
        if (!::WriteFile(m_pipe, message + offset, size - offset, &written, nullptr)) {
            break;
        }
        offset += written;
        if (offset == size) {
            sent = true;
            break;
        }
        if (::GetTickCount64() >= deadline) {
            break;
        }
        ::Sleep(5);
    }
    // The buffer held a password.
    ::SecureZeroMemory(message, sizeof(message));
    if (!sent) {
        end();
        return false;
    }
    // The helper answers once the batch is injected. Waiting for it keeps the
    // caller's pacing real, puts a dropped batch at the keystroke that dropped it,
    // and leaves nothing unread when the sequence ends.
    const char reply = awaitReply();
    if (reply == 'A') {
        return true;
    }
    if (reply == 'F') {
        // Target lost the foreground: the sequence stops, as it does without a
        // helper when the active window changes. Not a failure.
        m_interrupted = true;
    } else {
        qWarning("Auto-Type: the uiAccess helper did not confirm a batch (%s)",
                 reply == 'S' ? "SendInput dropped part of it"
                 : reply      ? "unexpected reply"
                              : "no reply in time");
    }
    end();
    return false;
}

char UiAccessInjector::awaitReply()
{
    const ULONGLONG deadline = ::GetTickCount64() + s_replyTimeoutMs;
    for (;;) {
        char code = 0;
        DWORD read = 0;
        if (::ReadFile(m_pipe, &code, 1, &read, nullptr) && read == 1) {
            return code;
        }
        // Non-blocking read with nothing to read: ERROR_NO_DATA. Anything else is the pipe gone.
        const DWORD error = ::GetLastError();
        if (read == 0 && error != ERROR_NO_DATA && error != ERROR_SUCCESS) {
            return 0;
        }
        if (::GetTickCount64() >= deadline) {
            return 0;
        }
        ::Sleep(1);
    }
}

void UiAccessInjector::end()
{
    // Describes the helper this call ends; a stale value would release modifiers
    // after unrelated sequences.
    m_helperFailed = false;
    if (m_pipe != INVALID_HANDLE_VALUE) {
        // Every sent batch was acknowledged: nothing unread is discarded here.
        ::DisconnectNamedPipe(m_pipe);
        ::CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
    if (m_process) {
        // Closing the pipe asks the helper to exit. Waited for in pumped slices: GUI thread.
        const ULONGLONG deadline = ::GetTickCount64() + s_helperExitTimeoutMs;
        bool exited = ::WaitForSingleObject(m_process, 0) == WAIT_OBJECT_0;
        while (!exited && ::GetTickCount64() < deadline) {
            Tools::wait(20);
            exited = ::WaitForSingleObject(m_process, 0) == WAIT_OBJECT_0;
        }
        if (!exited) {
            // PROCESS_TERMINATE on a uiAccess process is write-up and denied to this
            // one; the helper's idle backstop ends it then.
            if (::TerminateProcess(m_process, 1)) {
                qWarning("Auto-Type: the uiAccess helper did not exit; terminated");
            } else {
                qWarning("Auto-Type: the uiAccess helper did not exit and could not be terminated (%lu); "
                         "it ends on its idle timeout",
                         ::GetLastError());
            }
        }
        DWORD code = 0;
        const bool badExit = exited && ::GetExitCodeProcess(m_process, &code) && code != 0;
        // A target that lost the foreground was answered per batch and is not a
        // failure; any other exit that is not a clean 0 is.
        m_helperFailed = !m_interrupted && (!exited || badExit);
        if (m_helperFailed) {
            qWarning("Auto-Type: the uiAccess helper ended with %lu; some keystrokes may not have arrived",
                     exited ? code : 1u);
        }
        ::CloseHandle(m_process);
        m_process = nullptr;
    }
    m_target = nullptr;
}
