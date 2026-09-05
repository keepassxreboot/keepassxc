/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2016 Lennart Glauer <mail@lennart-glauer.de>
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

#ifndef KEEPASSXC_AUTOTYPEWINDOWS_H
#define KEEPASSXC_AUTOTYPEWINDOWS_H

#undef NOMINMAX
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "autotype/AutoTypeAction.h"
#include "autotype/AutoTypePlatform.h"

#include <QScopedPointer>

class WinUtils;
#include "config-keepassx.h"

#ifdef KPXC_FEATURE_UIACCESS_HELPER
class UiAccessInjector;
#endif

class AutoTypePlatformWin : public QObject, public AutoTypePlatformInterface
{
    Q_OBJECT

public:
    explicit AutoTypePlatformWin();
    // In the .cpp: QScopedPointer needs the complete type there.
    ~AutoTypePlatformWin() override;
    bool isAvailable() override;
    QStringList windowTitles() override;
    WId activeWindow() override;
    QString activeWindowTitle() override;
    bool raiseWindow(WId window) override;
    // Decided here rather than in raiseWindow; see AutoTypePlatform.h.
    void beginSequence(WId window) override;
    AutoTypeAction::Result endSequence() override;
    AutoTypeExecutor& executor() const override;

    void sendCharVirtual(const QChar& ch);
    void sendChar(const QChar& ch);
    void setKeyState(Qt::Key key, bool down);

private:
    AutoTypeExecutor* m_executor = nullptr;
#ifdef KPXC_FEATURE_UIACCESS_HELPER
    // Inactive when no helper is available. See UiAccessInjector.
    QScopedPointer<UiAccessInjector> m_injector;
#endif
    // Per sequence; no fallback to ::SendInput once delegated.
    bool m_delegating = false;
    bool m_delegationFailed = false;

public:
    /** Ok, or a failure when a delegated sequence stopped part-way. */
    AutoTypeAction::Result sequenceResult() const;

private:
    // The single exit for injected input.
    bool sendInputs(INPUT* inputs, int count);

    static bool isExtendedKey(DWORD nativeKeyCode);
    static bool isAltTabWindow(HWND hwnd);
    static BOOL CALLBACK windowTitleEnumProc(_In_ HWND hwnd, _In_ LPARAM lParam);
    static QString windowTitle(HWND hwnd);
};

class AutoTypeExecutorWin : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecutorWin(AutoTypePlatformWin* platform);

    AutoTypeAction::Result execBegin(const AutoTypeBegin* action) override;
    AutoTypeAction::Result execType(const AutoTypeKey* action) override;
    AutoTypeAction::Result execClearField(const AutoTypeClearField* action) override;

private:
    AutoTypePlatformWin* const m_platform;
};

#endif // KEEPASSXC_AUTOTYPEWINDOWS_H
