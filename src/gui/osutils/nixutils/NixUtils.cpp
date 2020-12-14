/*
 * Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "NixUtils.h"
#include "KeySymMap.h"
#include "core/Tools.h"

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPalette>
#include <QStandardPaths>
#include <QStyle>
#include <QTextStream>
#include <QtX11Extras/QX11Info>

#include <qpa/qplatformnativeinterface.h>

#include "X11Funcs.h"
#include <X11/XKBlib.h>
#include <xcb/xproto.h>

namespace
{
    Display* dpy;
    Window rootWindow;
    bool x11ErrorOccurred = false;

    int x11ErrorHandler(Display*, XErrorEvent*)
    {
        x11ErrorOccurred = true;
        return 1;
    }
} // namespace

QPointer<NixUtils> NixUtils::m_instance = nullptr;

NixUtils* NixUtils::instance()
{
    if (!m_instance) {
        m_instance = new NixUtils(qApp);
    }

    return m_instance;
}

NixUtils::NixUtils(QObject* parent)
    : OSUtilsBase(parent)
{
    dpy = QX11Info::display();
    rootWindow = QX11Info::appRootWindow();
}

NixUtils::~NixUtils()
{
}

bool NixUtils::isDarkMode() const
{
    if (!qApp || !qApp->style()) {
        return false;
    }
    return qApp->style()->standardPalette().color(QPalette::Window).toHsl().lightness() < 110;
}

QString NixUtils::getAutostartDesktopFilename(bool createDirs) const
{
    QDir autostartDir;
    auto confHome = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (confHome.isEmpty()) {
        return {};
    }
    autostartDir.setPath(confHome + QStringLiteral("/autostart"));
    if (createDirs && !autostartDir.exists()) {
        autostartDir.mkpath(".");
    }

    return QFile(autostartDir.absoluteFilePath(qApp->property("KPXC_QUALIFIED_APPNAME").toString().append(".desktop")))
        .fileName();
}

bool NixUtils::isLaunchAtStartupEnabled() const
{
    return QFile::exists(getAutostartDesktopFilename());
    ;
}

void NixUtils::setLaunchAtStartup(bool enable)
{
    if (enable) {
        QFile desktopFile(getAutostartDesktopFilename(true));
        if (!desktopFile.open(QIODevice::WriteOnly)) {
            qWarning("Failed to create autostart desktop file.");
            return;
        }
        QTextStream stream(&desktopFile);
        stream.setCodec("UTF-8");
        stream << QStringLiteral("[Desktop Entry]") << '\n'
               << QStringLiteral("Name=") << QApplication::applicationDisplayName() << '\n'
               << QStringLiteral("GenericName=") << tr("Password Manager") << '\n'
               << QStringLiteral("Exec=") << QApplication::applicationFilePath() << '\n'
               << QStringLiteral("TryExec=") << QApplication::applicationFilePath() << '\n'
               << QStringLiteral("Icon=") << QApplication::applicationName().toLower() << '\n'
               << QStringLiteral("StartupWMClass=keepassxc") << '\n'
               << QStringLiteral("StartupNotify=true") << '\n'
               << QStringLiteral("Terminal=false") << '\n'
               << QStringLiteral("Type=Application") << '\n'
               << QStringLiteral("Version=1.0") << "true" << '\n'
               << QStringLiteral("Categories=Utility;Security;Qt;") << '\n'
               << QStringLiteral("MimeType=application/x-keepass2;") << '\n'
               << QStringLiteral("X-GNOME-Autostart-enabled=true") << endl;
        desktopFile.close();
    } else if (isLaunchAtStartupEnabled()) {
        QFile::remove(getAutostartDesktopFilename());
    }
}

bool NixUtils::isCapslockEnabled()
{
    QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
    auto* display = native->nativeResourceForWindow("display", nullptr);
    if (!display) {
        return false;
    }

    QString platform = QGuiApplication::platformName();
    if (platform == "xcb") {
        unsigned state = 0;
        if (XkbGetIndicatorState(reinterpret_cast<Display*>(display), XkbUseCoreKbd, &state) == Success) {
            return ((state & 1u) != 0);
        }
    }

    // TODO: Wayland

    return false;
}

void NixUtils::registerNativeEventFilter()
{
    qApp->installNativeEventFilter(this);
}

bool NixUtils::nativeEventFilter(const QByteArray& eventType, void* message, long*)
{
    if (eventType != QByteArrayLiteral("xcb_generic_event_t")) {
        return false;
    }

    auto* genericEvent = static_cast<xcb_generic_event_t*>(message);
    quint8 type = genericEvent->response_type & 0x7f;

    if (type == XCB_KEY_PRESS) {
        auto* keyPressEvent = static_cast<xcb_key_press_event_t*>(message);
        auto modifierMask = ControlMask | ShiftMask | Mod1Mask | Mod4Mask;
        return triggerGlobalShortcut(keyPressEvent->detail, keyPressEvent->state & modifierMask);
    } else if (type == XCB_MAPPING_NOTIFY) {
        auto* mappingNotifyEvent = static_cast<xcb_mapping_notify_event_t*>(message);
        if (mappingNotifyEvent->request == XCB_MAPPING_KEYBOARD
            || mappingNotifyEvent->request == XCB_MAPPING_MODIFIER) {
            XMappingEvent xMappingEvent;
            memset(&xMappingEvent, 0, sizeof(xMappingEvent));
            xMappingEvent.type = MappingNotify;
            xMappingEvent.display = dpy;
            if (mappingNotifyEvent->request == XCB_MAPPING_KEYBOARD) {
                xMappingEvent.request = MappingKeyboard;
            } else {
                xMappingEvent.request = MappingModifier;
            }
            xMappingEvent.first_keycode = mappingNotifyEvent->first_keycode;
            xMappingEvent.count = mappingNotifyEvent->count;
            XRefreshKeyboardMapping(&xMappingEvent);
            // Notify listeners that the keymap has changed
            emit keymapChanged();
        }
    }

    return false;
}

bool NixUtils::triggerGlobalShortcut(uint keycode, uint modifiers)
{
    QHashIterator<QString, QSharedPointer<globalShortcut>> i(m_globalShortcuts);
    while (i.hasNext()) {
        i.next();
        if (i.value()->nativeKeyCode == keycode && i.value()->nativeModifiers == modifiers) {
            emit globalShortcutTriggered(i.key());
            return true;
        }
    }
    return false;
}

bool NixUtils::registerGlobalShortcut(const QString& name, Qt::Key key, Qt::KeyboardModifiers modifiers, QString* error)
{
    auto keycode = XKeysymToKeycode(dpy, qcharToNativeKeyCode(key));
    auto modifierscode = qtToNativeModifiers(modifiers);

    // Check if this key combo is registered to another shortcut
    QHashIterator<QString, QSharedPointer<globalShortcut>> i(m_globalShortcuts);
    while (i.hasNext()) {
        i.next();
        if (i.value()->nativeKeyCode == keycode && i.value()->nativeModifiers == modifierscode && i.key() != name) {
            if (error) {
                *error = tr("Global shortcut already registered to %1").arg(i.key());
            }
            return false;
        }
    }

    unregisterGlobalShortcut(name);

    x11ErrorOccurred = false;
    auto prevHandler = XSetErrorHandler(x11ErrorHandler);

    XGrabKey(dpy, keycode, modifierscode, rootWindow, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, keycode, modifierscode | Mod2Mask, rootWindow, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, keycode, modifierscode | LockMask, rootWindow, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, keycode, modifierscode | Mod2Mask | LockMask, rootWindow, True, GrabModeAsync, GrabModeAsync);

    XSync(dpy, False);
    XSetErrorHandler(prevHandler);

    if (x11ErrorOccurred) {
        x11ErrorOccurred = false;
        if (error) {
            *error = tr("Could not register global shortcut");
        }
        return false;
    }

    auto gs = QSharedPointer<globalShortcut>::create();
    gs->nativeKeyCode = keycode;
    gs->nativeModifiers = modifierscode;
    m_globalShortcuts.insert(name, gs);
    return true;
}

bool NixUtils::unregisterGlobalShortcut(const QString& name)
{
    if (!m_globalShortcuts.contains(name)) {
        return false;
    }

    auto gs = m_globalShortcuts.value(name);
    XUngrabKey(dpy, gs->nativeKeyCode, gs->nativeModifiers, rootWindow);
    XUngrabKey(dpy, gs->nativeKeyCode, gs->nativeModifiers | Mod2Mask, rootWindow);
    XUngrabKey(dpy, gs->nativeKeyCode, gs->nativeModifiers | LockMask, rootWindow);
    XUngrabKey(dpy, gs->nativeKeyCode, gs->nativeModifiers | Mod2Mask | LockMask, rootWindow);

    m_globalShortcuts.remove(name);
    return true;
}
