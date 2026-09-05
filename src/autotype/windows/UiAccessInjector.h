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

#ifndef KEEPASSXC_UIACCESSINJECTOR_H
#define KEEPASSXC_UIACCESSINJECTOR_H

#include <QString>
#include <windows.h>

/**
 * Delegates keystrokes to keepassxc-uiaccess-helper for the one window UIPI
 * shields from a medium-integrity process: the CredentialUIBroker credential
 * prompt (issue 12956). One helper per sequence; both ends verify each other; the
 * pipe admits only the current user.
 */
class UiAccessInjector
{
public:
    UiAccessInjector() = default;
    ~UiAccessInjector();

    Q_DISABLE_COPY(UiAccessInjector)

    /** The installed helper, or empty when there is none to trust. */
    static QString helperPath();

    /** True when @p window is the credential prompt CredentialUIBroker owns. */
    static bool isBrokeredCredentialDialog(HWND window);

    /**
     * Starts the helper for @p target.
     *
     * False when there is no helper, when Windows declined the grant, or when
     * it did not connect; the caller must then keep using SendInput.
     */
    bool begin(HWND target);

    /** True while a helper is connected and delegation is in effect. */
    bool active() const;

    /** Hands @p count INPUT records to the helper and waits for it to inject them. False: not injected. */
    bool send(const INPUT* inputs, int count);
    /** True when the helper stopped because the target lost the foreground: an interruption, not a failure. */
    bool interrupted() const
    {
        return m_interrupted;
    }

    /** Closes the pipe, which is how the helper is asked to exit. */
    void end();

    /** True when the helper this injector last ended failed; set by end(). */
    bool helperFailed() const
    {
        return m_helperFailed;
    }

private:
    /** The helper's one-byte answer to the last batch; 0 on timeout or a broken pipe. */
    char awaitReply();
    /** Waits for the helper to connect. Never blocks indefinitely. */
    bool waitForHelper();

    static constexpr int s_helperConnectTimeoutMs = 3000;
    static constexpr int s_helperConnectPollMs = 50;
    static constexpr int s_helperExitTimeoutMs = 2000;
    static constexpr int s_sendRetryMs = 500;
    static constexpr int s_replyTimeoutMs = 2000;

    bool m_helperFailed = false;
    bool m_interrupted = false;
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    HANDLE m_process = nullptr;
    HWND m_target = nullptr;
};

#endif // KEEPASSXC_UIACCESSINJECTOR_H
