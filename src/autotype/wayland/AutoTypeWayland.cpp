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

#include "AutoTypeWayland.h"
#include "autotype/AutoTypeAction.h"
#include "core/Tools.h"
#include "gui/osutils/nixutils/NixUtils.h"
#include "gui/osutils/nixutils/RemoteDesktopPortal.h"

#include <QApplication>

#include <xkbcommon/xkbcommon.h>

namespace
{
    xkb_keysym_t qtKeyToXkbKeysym(Qt::Key key)
    {
        switch (key) {
        case Qt::Key_Tab:
            return XKB_KEY_Tab;
        case Qt::Key_Enter:
            return XKB_KEY_Return;
        case Qt::Key_Space:
            return XKB_KEY_space;
        case Qt::Key_Up:
            return XKB_KEY_Up;
        case Qt::Key_Down:
            return XKB_KEY_Down;
        case Qt::Key_Left:
            return XKB_KEY_Left;
        case Qt::Key_Right:
            return XKB_KEY_Right;
        case Qt::Key_Insert:
            return XKB_KEY_Insert;
        case Qt::Key_Delete:
            return XKB_KEY_Delete;
        case Qt::Key_Home:
            return XKB_KEY_Home;
        case Qt::Key_End:
            return XKB_KEY_End;
        case Qt::Key_PageUp:
            return XKB_KEY_Page_Up;
        case Qt::Key_PageDown:
            return XKB_KEY_Page_Down;
        case Qt::Key_Backspace:
            return XKB_KEY_BackSpace;
        case Qt::Key_Pause:
            return XKB_KEY_Break;
        case Qt::Key_CapsLock:
            return XKB_KEY_Caps_Lock;
        case Qt::Key_Escape:
            return XKB_KEY_Escape;
        case Qt::Key_Help:
            return XKB_KEY_Help;
        case Qt::Key_NumLock:
            return XKB_KEY_Num_Lock;
        case Qt::Key_Print:
            return XKB_KEY_Print;
        case Qt::Key_ScrollLock:
            return XKB_KEY_Scroll_Lock;
        case Qt::Key_Shift:
            return XKB_KEY_Shift_L;
        case Qt::Key_Control:
            return XKB_KEY_Control_L;
        case Qt::Key_Alt:
            return XKB_KEY_Alt_L;
        default:
            if (key >= Qt::Key_F1 && key <= Qt::Key_F16) {
                return XKB_KEY_F1 + (key - Qt::Key_F1);
            } else if (key >= Qt::Key_Space && key <= Qt::Key_AsciiTilde) {
                return key & 0xff;
            } else {
                return XKB_KEY_NoSymbol;
            }
        }
    }
} // namespace

void AutoTypePlatformWayland::setOSUtils(OSUtilsBase* osUtils)
{
    m_nixUtils = static_cast<NixUtils*>(osUtils);
}

void AutoTypePlatformWayland::prepareAutoType()
{
    m_nixUtils->remoteDesktopPortal()->tryStartSession();
}

void AutoTypePlatformWayland::finishAutoType()
{
    m_nixUtils->remoteDesktopPortal()->finishSession();
}

bool AutoTypePlatformWayland::isAvailable()
{
    return m_nixUtils->remoteDesktopPortal()->isAvailable();
}

void AutoTypePlatformWayland::unload()
{
    m_nixUtils->remoteDesktopPortal()->closeSession();
}

QStringList AutoTypePlatformWayland::windowTitles()
{
    return {};
}

WId AutoTypePlatformWayland::activeWindow()
{
    // return a different value if the focus changes to our own window to stop sequence
    if (qApp->activeWindow()) {
        return -2;
    }

    // return non-zero value to avoid minimizing our own window if it's not in focus
    return -1;
}

QString AutoTypePlatformWayland::activeWindowTitle()
{
    return {};
}

bool AutoTypePlatformWayland::raiseWindow(WId window)
{
    Q_UNUSED(window);

    return true;
}

AutoTypeAction::Result AutoTypePlatformWayland::sendKey(const AutoTypeKey* action)
{
    xkb_keysym_t keysym;
    if (action->key != Qt::Key_unknown) {
        keysym = qtKeyToXkbKeysym(action->key);
        if (keysym == XKB_KEY_NoSymbol) {
            return AutoTypeAction::Result::Failed(tr("No symbol found for key: '%1'").arg(action->key));
        }
    } else {
        keysym = xkb_utf32_to_keysym(action->character.unicode());
        if (keysym == XKB_KEY_NoSymbol) {
            return AutoTypeAction::Result::Failed(tr("No symbol found for character: '%1'").arg(action->character));
        }
    }

    QVector<int> modKeys{};
    if (action->modifiers & Qt::ShiftModifier) {
        modKeys.append(XKB_KEY_Shift_L);
    }
    if (action->modifiers & Qt::ControlModifier) {
        modKeys.append(XKB_KEY_Control_L);
    }
    if (action->modifiers & Qt::AltModifier) {
        modKeys.append(XKB_KEY_Alt_L);
    }
    if (action->modifiers & Qt::MetaModifier) {
        modKeys.append(XKB_KEY_Meta_L);
    }

    QString error;
    if (!m_nixUtils->remoteDesktopPortal()->sendKeysym(keysym, modKeys, error)) {
        return AutoTypeAction::Result::Failed(error);
    }

    return AutoTypeAction::Result::Ok();
}

AutoTypeExecutor* AutoTypePlatformWayland::createExecutor()
{
    return new AutoTypeExecutorWayland(this);
}

AutoTypeExecutorWayland::AutoTypeExecutorWayland(AutoTypePlatformWayland* platform)
    : m_platform(platform)
{
}

AutoTypeAction::Result AutoTypeExecutorWayland::execBegin(const AutoTypeBegin* action)
{
    Q_UNUSED(action);

    auto* portal = m_platform->m_nixUtils->remoteDesktopPortal();
    portal->waitForSession();

    auto error = portal->errorString();
    if (!error.isEmpty()) {
        return AutoTypeAction::Result::Failed(error);
    }

    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorWayland::execType(const AutoTypeKey* action)
{
    auto result = m_platform->sendKey(action);

    if (result.isOk()) {
        Tools::sleep(execDelayMs);
    }

    return result;
}

AutoTypeAction::Result AutoTypeExecutorWayland::execClearField(const AutoTypeClearField* action)
{
    Q_UNUSED(action);
    AutoTypeKey home(Qt::Key_Home);
    AutoTypeKey end(Qt::Key_End, Qt::ShiftModifier);
    AutoTypeKey backspace(Qt::Key_Backspace);
    execType(&home);
    execType(&end);
    execType(&backspace);
    return AutoTypeAction::Result::Ok();
}
