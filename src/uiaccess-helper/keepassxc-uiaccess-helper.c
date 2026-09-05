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

/*
 * Injects keystrokes into the "Windows Security" credential dialog on behalf of
 * KeePassXC. UIPI discards SendInput from a medium-integrity process to that
 * dialog (issue 12956); this binary carries uiAccess, which requires its own signed
 * executable under a protected path (13070, 13116) and a ShellExecute launch.
 *
 * Scope is fixed at startup and re-checked per batch: the pipe server must be
 * the KeePassXC beside this binary, the target must be a foreground credential
 * dialog owned by CredentialUIBroker, batches are bounded, nothing is written
 * to disk, and the process exits with the pipe.
 *
 * Usage: keepassxc-uiaccess-helper.exe --pipe <name> --target <hwnd>
 */

/* FILE_ID_INFO needs the Windows 8 SDK surface. */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif
#include <windows.h>

#include <shellapi.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/* One Auto-Type action at a time; the caller splits longer sequences. */
#define MAX_BATCH 64

/* Backstop for a caller that hangs holding the pipe; a sequence normally ends
 * with the pipe. Long, because {DELAY} tokens are unbounded in count. */
#define MAX_IDLE_MS 600000
#define POLL_MS 25

/* Nothing is injected into a window other than the one named at startup. */
static HWND g_target = NULL;

static void report(const char* format, ...)
{
    char message[512];
    va_list args;
    va_start(args, format);
    _vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);
    va_end(args);
    OutputDebugStringA(message);
}

/* Read, not assumed: without the grant every SendInput is silently discarded. */
static DWORD ui_access_flag(void)
{
    HANDLE token = NULL;
    DWORD ui_access = 0;
    DWORD size = 0;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        /* TokenUIAccess == 26; older SDK headers have no enumerator for it. */
        if (!GetTokenInformation(token, (TOKEN_INFORMATION_CLASS)26, &ui_access, sizeof(ui_access), &size)) {
            report("keepassxc-uiaccess-helper: TokenUIAccess unreadable: %lu\n", GetLastError());
        }
        CloseHandle(token);
    }
    return ui_access;
}

/* The caller supplies only the name, so it may not escape the pipe namespace. */
static int valid_pipe_name(const wchar_t* name)
{
    size_t length = 0;
    if (!name || !*name) {
        return 0;
    }
    for (const wchar_t* c = name; *c; ++c) {
        if (*c == L'\\' || *c == L'/' || *c < 0x20) {
            return 0;
        }
        if (++length > 64) {
            return 0;
        }
    }
    return 1;
}

static int image_path_of(DWORD pid, wchar_t* out, DWORD chars)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return 0;
    }
    DWORD size = chars;
    const int ok = QueryFullProcessImageNameW(process, 0, out, &size) ? 1 : 0;
    CloseHandle(process);
    return ok;
}

/* Same file regardless of spelling (8.3, SUBST, junction): volume and file id,
 * not paths. */
static HANDLE open_for_identity(const wchar_t* path)
{
    /* FILE_SHARE_DELETE: another opener with delete sharing must not fail this open. */
    return CreateFileW(path,
                       0,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS,
                       NULL);
}

static int same_file(const wchar_t* left, const wchar_t* right)
{
    HANDLE first = open_for_identity(left);
    if (first == INVALID_HANDLE_VALUE) {
        return 0;
    }
    HANDLE second = open_for_identity(right);
    if (second == INVALID_HANDLE_VALUE) {
        CloseHandle(first);
        return 0;
    }
    int ok = 0;
    /* 128-bit ids where supported (ReFS truncates the 64-bit view); 64-bit otherwise. */
    FILE_ID_INFO ia;
    FILE_ID_INFO ib;
    if (GetFileInformationByHandleEx(first, FileIdInfo, &ia, sizeof(ia))
        && GetFileInformationByHandleEx(second, FileIdInfo, &ib, sizeof(ib))) {
        ok = ia.VolumeSerialNumber == ib.VolumeSerialNumber && memcmp(&ia.FileId, &ib.FileId, sizeof(ia.FileId)) == 0;
    } else {
        BY_HANDLE_FILE_INFORMATION a;
        BY_HANDLE_FILE_INFORMATION b;
        ok = GetFileInformationByHandle(first, &a) && GetFileInformationByHandle(second, &b)
             && a.dwVolumeSerialNumber == b.dwVolumeSerialNumber && a.nFileIndexHigh == b.nFileIndexHigh
             && a.nFileIndexLow == b.nFileIndexLow;
    }
    CloseHandle(second);
    CloseHandle(first);
    return ok;
}

static int server_is_our_application(HANDLE pipe)
{
    DWORD server_pid = 0;
    if (!GetNamedPipeServerProcessId(pipe, &server_pid)) {
        report("keepassxc-uiaccess-helper: cannot identify the pipe server: %lu\n", GetLastError());
        return 0;
    }

    wchar_t expected[MAX_PATH] = {0};
    const DWORD own = GetModuleFileNameW(NULL, expected, MAX_PATH);
    if (own == 0 || own >= MAX_PATH) {
        return 0; /* truncated: not something to compare paths against */
    }
    wchar_t* slash = wcsrchr(expected, L'\\');
    if (!slash) {
        return 0;
    }
    if (_snwprintf_s(slash + 1, MAX_PATH - (size_t)(slash + 1 - expected), _TRUNCATE, L"KeePassXC.exe") < 0) {
        return 0;
    }

    /* Held for the whole check, so the id cannot be reused mid-check. */
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, server_pid);
    if (!process) {
        return 0;
    }

    wchar_t actual[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    if (!QueryFullProcessImageNameW(process, 0, actual, &size)) {
        CloseHandle(process);
        return 0;
    }
    if (!same_file(actual, expected)) {
        report("keepassxc-uiaccess-helper: refusing a pipe served by %ls\n", actual);
        CloseHandle(process);
        return 0;
    }

    CloseHandle(process);
    return 1;
}

/* The scope is fixed here; the caller cannot widen it. */
static int target_is_credential_dialog(HWND window)
{
    wchar_t class_name[64] = {0};
    if (!GetClassNameW(window, class_name, 64)) {
        return 0;
    }
    if (wcscmp(class_name, L"Credential Dialog Xaml Host") != 0) {
        return 0;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    wchar_t image[MAX_PATH] = {0};
    if (!image_path_of(pid, image, MAX_PATH)) {
        return 0;
    }
    /* Not GetSystemDirectoryW: redirected to SysWOW64 for a 32-bit process. */
    wchar_t expected[MAX_PATH] = {0};
    const UINT length = GetWindowsDirectoryW(expected, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return 0;
    }
    if (_snwprintf_s(expected + length, MAX_PATH - length, _TRUNCATE, L"\\System32\\CredentialUIBroker.exe") < 0) {
        return 0;
    }
    /* Runs per batch: equal text needs no file-identity check. */
    if (_wcsicmp(image, expected) == 0) {
        return 1;
    }
    if (!same_file(image, expected)) {
        return 0;
    }
    return 1;
}

/* Releases the modifiers an aborted sequence may have left down. Not on a normal
 * ending: the user may still hold the key that started Auto-Type. No Windows
 * keys: a key-up can open Start. */
static void release_modifiers(void)
{
    static const WORD keys[] = {
        VK_SHIFT, VK_CONTROL, VK_MENU, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU};
    INPUT up[sizeof(keys) / sizeof(keys[0])];
    ZeroMemory(up, sizeof(up));
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        up[i].type = INPUT_KEYBOARD;
        up[i].ki.wVk = keys[i];
        up[i].ki.dwFlags = KEYEVENTF_KEYUP;
    }
    SendInput((UINT)(sizeof(up) / sizeof(up[0])), up, sizeof(INPUT));
}

/* One byte back per batch: 'A' injected, 'F' target lost the foreground, 'S'
 * SendInput dropped part of it. The caller waits for it before the next batch,
 * so pacing and failure both land at the keystroke. */
static void reply(HANDLE pipe, char code)
{
    DWORD written = 0;
    WriteFile(pipe, &code, 1, &written, NULL);
}

/* Key-ups alone cannot type into the wrong window; they are released even after
 * the target is gone, so a modifier or the Enter that submitted the prompt does
 * not stay down. */
static int keyups_only(const INPUT* batch, DWORD count)
{
    for (DWORD i = 0; i < count; ++i) {
        if (batch[i].type != INPUT_KEYBOARD || !(batch[i].ki.dwFlags & KEYEVENTF_KEYUP)) {
            return 0;
        }
    }
    return 1;
}

static int pipe_mode(const wchar_t* name)
{
    wchar_t path[MAX_PATH];
    if (_snwprintf_s(path, MAX_PATH, _TRUNCATE, L"\\\\.\\pipe\\%ls", name) < 0) {
        return 4;
    }

    /* SECURITY_ANONYMOUS: the server cannot impersonate this process before it is
     * verified. Retried on ERROR_PIPE_BUSY: one stray client must not end this
     * process. */
    HANDLE pipe = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 20; ++attempt) {
        pipe = CreateFileW(path,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS,
                           NULL);
        if (pipe != INVALID_HANDLE_VALUE || GetLastError() != ERROR_PIPE_BUSY) {
            break;
        }
        WaitNamedPipeW(path, 50);
    }
    if (pipe == INVALID_HANDLE_VALUE) {
        report("keepassxc-uiaccess-helper: pipe open failed: %lu\n", GetLastError());
        return 4;
    }

    if (!server_is_our_application(pipe)) {
        CloseHandle(pipe);
        return 6;
    }

    /* Polled: a blocking read would outlive a caller that hangs holding the pipe. */
    DWORD idle = 0;
    for (;;) {
        DWORD available = 0;
        if (!PeekNamedPipe(pipe, NULL, 0, NULL, &available, NULL)) {
            break; /* the caller closed the pipe: the sequence is over */
        }
        DWORD count = 0;
        DWORD peeked = 0;
        if (available >= sizeof(count) && !PeekNamedPipe(pipe, &count, sizeof(count), &peeked, NULL, NULL)) {
            break;
        }
        if (count > MAX_BATCH) {
            /* Every abort returns non-zero: the exit code is the only channel that
             * can report a dropped last batch. */
            report("keepassxc-uiaccess-helper: refusing a batch of %lu records\n", count);
            release_modifiers();
            CloseHandle(pipe);
            return 7;
        }

        /* Consume nothing until the whole message is here, or a caller that stops
         * mid-message parks this process past MAX_IDLE_MS. */
        const DWORD wanted = count * (DWORD)sizeof(INPUT);
        if (available >= sizeof(count) && count == 0) {
            report("keepassxc-uiaccess-helper: refusing a batch of 0 records\n");
            release_modifiers();
            CloseHandle(pipe);
            return 7;
        }
        if (available < sizeof(count) || available < sizeof(count) + wanted) {
            if (idle >= MAX_IDLE_MS) {
                report("keepassxc-uiaccess-helper: idle for %lums, exiting\n", idle);
                release_modifiers();
                CloseHandle(pipe);
                return 8;
            }
            Sleep(POLL_MS);
            idle += POLL_MS;
            continue;
        }
        idle = 0;

        INPUT batch[MAX_BATCH];
        DWORD got = 0;
        if (!ReadFile(pipe, &count, sizeof(count), &got, NULL) || got != sizeof(count)) {
            break;
        }
        DWORD total = 0;
        while (total < wanted) {
            if (!ReadFile(pipe, ((char*)batch) + total, wanted - total, &got, NULL) || got == 0) {
                report("keepassxc-uiaccess-helper: short read, %lu of %lu\n", total, wanted);
                SecureZeroMemory(batch, total);
                release_modifiers();
                CloseHandle(pipe);
                return 5;
            }
            total += got;
        }

        /* Per batch: a window handle can be reused after the dialog is destroyed. */
        if (!keyups_only(batch, count)
            && (GetForegroundWindow() != g_target || !target_is_credential_dialog(g_target))) {
            /* Abort, never skip: a skipped batch is invisible to the caller and the
             * rest of the sequence still arrives, possibly into the wrong field. */
            report("keepassxc-uiaccess-helper: target lost the foreground, aborting the sequence\n");
            SecureZeroMemory(batch, wanted);
            release_modifiers();
            reply(pipe, 'F');
            CloseHandle(pipe);
            return 9;
        }

        const UINT sent = SendInput(count, batch, sizeof(INPUT));
        /* The batch held a password; nothing reads it again. */
        SecureZeroMemory(batch, wanted);
        if (sent != count) {
            report("keepassxc-uiaccess-helper: SendInput queued %u of %lu: %lu\n", sent, count, GetLastError());
            release_modifiers();
            reply(pipe, 'S');
            CloseHandle(pipe);
            return 10;
        }
        reply(pipe, 'A');
    }

    CloseHandle(pipe);
    return 0;
}

/* Windowed (no console on the taskbar); a windowed entry point has no argv. */
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show)
{
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(previous);
    UNREFERENCED_PARAMETER(command_line);
    UNREFERENCED_PARAMETER(show);

    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return 2;
    }

    const wchar_t* name = NULL;
    for (int i = 1; i + 1 < argc; ++i) {
        if (wcscmp(argv[i], L"--pipe") == 0) {
            name = argv[++i];
        } else if (wcscmp(argv[i], L"--target") == 0) {
            g_target = (HWND)(uintptr_t)_wcstoui64(argv[++i], NULL, 10);
        }
    }

    /* Without a target this would inject into whatever holds the foreground. */
    if (!valid_pipe_name(name) || !g_target) {
        report("keepassxc-uiaccess-helper: usage: --pipe <name> --target <hwnd>\n");
        LocalFree(argv);
        return 2;
    }

    /* Without uiAccess every SendInput is discarded while the caller believes it
     * was delivered. */
    if (!ui_access_flag()) {
        report("keepassxc-uiaccess-helper: uiAccess was not granted\n");
        LocalFree(argv);
        return 3;
    }

    if (!target_is_credential_dialog(g_target)) {
        report("keepassxc-uiaccess-helper: the target is not a brokered credential dialog\n");
        LocalFree(argv);
        return 3;
    }

    report("keepassxc-uiaccess-helper: target=%p\n", (void*)g_target);
    const int rc = pipe_mode(name);
    LocalFree(argv);
    return rc;
}
