/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "Bootstrap.h"
#include "config-keepassx.h"
#include "core/Config.h"
#include "core/Translator.h"
#include "gui/MessageBox.h"

#ifdef Q_OS_WIN
#include <aclapi.h> // for createWindowsDACL()
#include <windows.h> // for Sleep(), SetDllDirectoryA(), SetSearchPathMode(), ...
#undef MessageBox
#endif

#if defined(HAVE_RLIMIT_CORE)
#include <sys/resource.h>
#endif

#if defined(HAVE_PR_SET_DUMPABLE)
#include <sys/prctl.h>
#endif

#ifdef HAVE_PT_DENY_ATTACH
// clang-format off
#include <sys/types.h>
#include <sys/ptrace.h>
// clang-format on
#endif

namespace Bootstrap
{
    /**
     * When QNetworkAccessManager is instantiated it regularly starts polling
     * all network interfaces to see if anything changes and if so, what. This
     * creates a latency spike every 10 seconds on Mac OS 10.12+ and Windows 7 >=
     * when on a wifi connection.
     * So here we disable it for lack of better measure.
     * This will also cause this message: QObject::startTimer: Timers cannot
     * have negative intervals
     * For more info see:
     * - https://bugreports.qt.io/browse/QTBUG-40332
     * - https://bugreports.qt.io/browse/QTBUG-46015
     */
    static inline void applyEarlyQNetworkAccessManagerWorkaround()
    {
        qputenv("QT_BEARER_POLL_TIMEOUT", QByteArray::number(-1));
    }

    /**
     * Perform early application bootstrapping that does not rely on a QApplication
     * being present.
     */
    void bootstrap()
    {
#ifdef QT_NO_DEBUG
        disableCoreDumps();
#endif
        setupSearchPaths();
        applyEarlyQNetworkAccessManagerWorkaround();
        Translator::installTranslators();
    }

    /**
     * Perform early application bootstrapping such as setting up search paths,
     * configuration OS security properties, and loading translators.
     * A QApplication object has to be instantiated before calling this function.
     */
    void bootstrapApplication()
    {
        bootstrap();
        MessageBox::initializeButtonDefs();

#ifdef Q_OS_MACOS
        // Don't show menu icons on OSX
        QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    }

    /**
     * Restore the main window's state after launch
     *
     * @param mainWindow the main window whose state to restore
     */
    void restoreMainWindowState(MainWindow& mainWindow)
    {
        // start minimized if configured
        if (config()->get("GUI/MinimizeOnStartup").toBool()) {
            mainWindow.showMinimized();
        } else {
            mainWindow.bringToFront();
        }

        if (config()->get("OpenPreviousDatabasesOnStartup").toBool()) {
            const QStringList fileNames = config()->get("LastOpenedDatabases").toStringList();
            for (const QString& filename : fileNames) {
                if (!filename.isEmpty() && QFile::exists(filename)) {
                    mainWindow.openDatabase(filename);
                }
            }
        }
    }

    // LCOV_EXCL_START
    void disableCoreDumps()
    {
        // default to true
        // there is no point in printing a warning if this is not implemented on the platform
        bool success = true;

#if defined(HAVE_RLIMIT_CORE)
        struct rlimit limit;
        limit.rlim_cur = 0;
        limit.rlim_max = 0;
        success = success && (setrlimit(RLIMIT_CORE, &limit) == 0);
#endif

#if defined(HAVE_PR_SET_DUMPABLE)
        success = success && (prctl(PR_SET_DUMPABLE, 0) == 0);
#endif

// Mac OS X
#ifdef HAVE_PT_DENY_ATTACH
        success = success && (ptrace(PT_DENY_ATTACH, 0, 0, 0) == 0);
#endif

#ifdef Q_OS_WIN
        success = success && createWindowsDACL();
#endif

        if (!success) {
            qWarning("Unable to disable core dumps.");
        }
    }

    //
    // This function grants the user associated with the process token minimal access rights and
    // denies everything else on Windows. This includes PROCESS_QUERY_INFORMATION and
    // PROCESS_VM_READ access rights that are required for MiniDumpWriteDump() or ReadProcessMemory().
    // We do this using a discretionary access control list (DACL). Effectively this prevents
    // crash dumps and disallows other processes from accessing our memory. This works as long
    // as you do not have admin privileges, since then you are able to grant yourself the
    // SeDebugPrivilege or SeTakeOwnershipPrivilege and circumvent the DACL.
    //
    bool createWindowsDACL()
    {
        bool bSuccess = false;

#ifdef Q_OS_WIN
        // Process token and user
        HANDLE hToken = nullptr;
        PTOKEN_USER pTokenUser = nullptr;
        DWORD cbBufferSize = 0;
        PSID pLocalSystemSid = nullptr;
        DWORD pLocalSystemSidSize = SECURITY_MAX_SID_SIZE;

        // Access control list
        PACL pACL = nullptr;
        DWORD cbACL = 0;

        // Open the access token associated with the calling process
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            goto Cleanup;
        }

        // Retrieve the token information in a TOKEN_USER structure
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &cbBufferSize);

        pTokenUser = static_cast<PTOKEN_USER>(HeapAlloc(GetProcessHeap(), 0, cbBufferSize));
        if (pTokenUser == nullptr) {
            goto Cleanup;
        }

        if (!GetTokenInformation(hToken, TokenUser, pTokenUser, cbBufferSize, &cbBufferSize)) {
            goto Cleanup;
        }

        if (!IsValidSid(pTokenUser->User.Sid)) {
            goto Cleanup;
        }

        // Retrieve LocalSystem account SID
        pLocalSystemSid = static_cast<PSID>(HeapAlloc(GetProcessHeap(), 0, pLocalSystemSidSize));
        if (pLocalSystemSid == nullptr) {
            goto Cleanup;
        }

        if (!CreateWellKnownSid(WinLocalSystemSid, nullptr, pLocalSystemSid, &pLocalSystemSidSize)) {
            goto Cleanup;
        }

        // Calculate the amount of memory that must be allocated for the DACL
        cbACL = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pTokenUser->User.Sid)
                + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pLocalSystemSid);

        // Create and initialize an ACL
        pACL = static_cast<PACL>(HeapAlloc(GetProcessHeap(), 0, cbACL));
        if (pACL == nullptr) {
            goto Cleanup;
        }

        if (!InitializeAcl(pACL, cbACL, ACL_REVISION)) {
            goto Cleanup;
        }

        // Add allowed access control entries, everything else is denied
        if (!AddAccessAllowedAce(
                pACL,
                ACL_REVISION,
                SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, // same as protected process
                pTokenUser->User.Sid // pointer to the trustee's SID
                )) {
            goto Cleanup;
        }

#ifdef WITH_XC_SSHAGENT
        // OpenSSH for Windows ssh-agent service is running as LocalSystem
        if (!AddAccessAllowedAce(pACL,
                                 ACL_REVISION,
                                 PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, // just enough for ssh-agent
                                 pLocalSystemSid // known LocalSystem sid
                                 )) {
            goto Cleanup;
        }
#endif

        // Set discretionary access control list
        bSuccess = ERROR_SUCCESS
                   == SetSecurityInfo(GetCurrentProcess(), // object handle
                                      SE_KERNEL_OBJECT, // type of object
                                      DACL_SECURITY_INFORMATION, // change only the objects DACL
                                      nullptr,
                                      nullptr, // do not change owner or group
                                      pACL, // DACL specified
                                      nullptr // do not change SACL
                      );

    Cleanup:

        if (pACL != nullptr) {
            HeapFree(GetProcessHeap(), 0, pACL);
        }
        if (pLocalSystemSid != nullptr) {
            HeapFree(GetProcessHeap(), 0, pLocalSystemSid);
        }
        if (pTokenUser != nullptr) {
            HeapFree(GetProcessHeap(), 0, pTokenUser);
        }
        if (hToken != nullptr) {
            CloseHandle(hToken);
        }
#endif

        return bSuccess;
    }
    // LCOV_EXCL_STOP

    void setupSearchPaths()
    {
#ifdef Q_OS_WIN
        // Make sure Windows doesn't load DLLs from the current working directory
        SetDllDirectoryA("");
        SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE);
#endif
    }

} // namespace Bootstrap
