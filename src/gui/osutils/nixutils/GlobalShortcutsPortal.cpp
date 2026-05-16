/*
 * Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "GlobalShortcutsPortal.h"

#include "core/Config.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "xdp_globalshortcuts.h"

#include <QDebug>
#include <QTimer>

Q_GLOBAL_STATIC_WITH_ARGS(OrgFreedesktopPortalGlobalShortcutsInterface,
                          s_shortcutsInterface,
                          ("org.freedesktop.portal.Desktop",
                           "/org/freedesktop/portal/desktop",
                           QDBusConnection::sessionBus()));

using XdpShortcut = QPair<QString, QVariantMap>;
using XdpShortcuts = QList<XdpShortcut>;

GlobalShortcutsPortal::GlobalShortcutsPortal(QObject* parent)
    : DesktopPortal(parent)
{
    if (!isAvailable()) {
        return;
    }

    qDBusRegisterMetaType<XdpShortcut>();
    qDBusRegisterMetaType<XdpShortcuts>();

    connect(s_shortcutsInterface,
            &OrgFreedesktopPortalGlobalShortcutsInterface::Activated,
            this,
            [this](const QDBusObjectPath& sessionHandle,
                   const QString& shortcutId,
                   qulonglong timestamp,
                   const QVariantMap& options) {
                Q_UNUSED(timestamp)
                Q_UNUSED(options)

                if (!hasSession()) {
                    qWarning() << "Global shortcut activated without session";
                } else if (!isCurrentSession(sessionHandle)) {
                    qWarning() << "Global shortcut activated for wrong session:" << sessionHandle.path()
                               << "!=" << sessionPath().path();
                } else if (shortcutId == "autotype") {
                    emit globalShortcutTriggered(shortcutId);
                } else {
                    qWarning() << "Unknown global shortcut activated:" << shortcutId;
                }
            });

    connect(s_shortcutsInterface,
            &OrgFreedesktopPortalGlobalShortcutsInterface::ShortcutsChanged,
            this,
            [this](const QDBusObjectPath& sessionHandle, const XdpShortcuts& shortcuts) {
                if (!isCurrentSession(sessionHandle)) {
                    return;
                }
                for (const auto& [id, map] : shortcuts) {
                    emit globalShortcutChanged(id, map.value(QStringLiteral("trigger_description")).toString());
                }
            });

    createSession();
}

bool GlobalShortcutsPortal::isAvailable() const
{
    return s_shortcutsInterface->isValid();
}

void GlobalShortcutsPortal::configureShortcuts()
{
    if (!isAvailable() || !hasSession()) {
        MessageBox::warning(getMainWindow(),
                            tr("KeePassXC - Global Shortcuts"),
                            tr("The XDG Desktop Portal for global shortcuts is not available on this system."));
        return;
    }

    // Use the config flag to determine if shortcuts have been configured before.
    // We cannot use ListShortcuts here because on GNOME, ListShortcuts always returns empty
    // even after BindShortcuts was called (newer GNOME auto-applies previously saved shortcuts
    // silently without populating the session's shortcuts array). Relying on ListShortcuts
    // would cause BindShortcuts to be called again, but the portal rejects it (bound=true),
    // resulting in no dialog being shown. On KDE, if this flag gets out-of-sync it doesn't matter because on startup
    // we'll call BindShortcuts if ListShortcuts returns an empty list so the shortcuts will be bound before we get
    // here.
    if (config()->get(Config::GUI_XDPGlobalShortcutsConfigured).toBool()) {
        // Already configured: portal v2+ can open a reconfiguration dialog directly
        if (s_shortcutsInterface->version() >= 2) {
            auto handleToken = portalRequest([](uint response, const QVariantMap&) { Q_UNUSED(response); });
            auto reply = s_shortcutsInterface->ConfigureShortcuts(
                sessionPath(), "", {{QLatin1String("handle_token"), handleToken}});
            reply.waitForFinished();
            if (!reply.isError()) {
                return;
            }

            // Additionally, the frontend portal may expose v2 but the backend implementation doesn't support it so the
            // method call can fail anyway
            qWarning() << "GlobalShortcutsPortal::configureShortcut ConfigureShortcuts failed, falling through to "
                          "dialog:"
                       << reply.error().message();
        }

        // Portal v1 (e.g. GNOME): no reconfiguration API, direct the user to system settings
        MessageBox::information(getMainWindow(),
                                tr("KeePassXC - Global Shortcuts"),
                                tr("Global Auto-Type shortcut is already configured. "
                                   "To change it, open your system settings and navigate to the "
                                   "keyboard or application shortcuts section."));
        return;
    }

    // First-time setup: use BindShortcuts to present the user with a system dialog.
    // bindShortcutsToCurrentSession() skips BindShortcuts when the config flag is false,
    // so the session's bound flag is still false and this call will succeed.
    callBindShortcuts();
}

void GlobalShortcutsPortal::onSessionClosed(const QVariantMap& details)
{
    Q_UNUSED(details)

    QTimer::singleShot(1000, this, &GlobalShortcutsPortal::createSession);
}

void GlobalShortcutsPortal::createSession()
{
    if (!isAvailable() || hasSession()) {
        return;
    }

    auto sessionHandleToken = DesktopPortal::prepareSession();
    auto expectedSessionPath = sessionPath();
    auto handleToken = portalRequest([this, expectedSessionPath](uint createSessionResponse,
                                                                 const QVariantMap& results) {
        if (!isCurrentSession(expectedSessionPath)) {
            return;
        }

        if (createSessionResponse != 0) {
            qWarning() << "GlobalShortcutsPortal::createSession CreateSession got unexpected response from portal:"
                       << createSessionResponse;
            closeSession();
            return;
        }

        auto sessionHandle = results.value("session_handle").toString();
        if (!sessionHandle.isEmpty() && sessionHandle != expectedSessionPath.path()) {
            qWarning() << "GlobalShortcutsPortal::createSession returned session handle does not match expected path:"
                       << sessionHandle << "!=" << expectedSessionPath.path();
            closeSession();
            return;
        }

        bindShortcutsToCurrentSession();
    });

    auto reply = s_shortcutsInterface->CreateSession({
        {QLatin1String("session_handle_token"), sessionHandleToken},
        {QLatin1String("handle_token"), handleToken},
    });
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "Failed to create Global Shortcuts session" << reply.error().message();
        if (isCurrentSession(expectedSessionPath)) {
            closeSession();
        }
    }
}

void GlobalShortcutsPortal::bindShortcutsToCurrentSession()
{
    // Only attempt to restore bindings if previously configured
    if (!config()->get(Config::GUI_XDPGlobalShortcutsConfigured).toBool()) {
        return;
    }

    // Use ListShortcuts to check if shortcuts are already active (e.g. KDE session restoration), on GNOME we need to
    // rebind them every time
    auto listToken = portalRequest([this](uint listResponse, const QVariantMap& listResults) {
        if (listResponse != 0) {
            qWarning() << "GlobalShortcutsPortal::bindShortcutsToCurrentSession ListShortcuts failed with response:"
                       << listResponse;
            return;
        }

        auto existing = qdbus_cast<XdpShortcuts>(listResults.value("shortcuts"));
        if (!existing.isEmpty()) {
            for (const auto& [id, map] : existing) {
                emit globalShortcutChanged(id, map.value(QStringLiteral("trigger_description")).toString());
            }
            return; // already active, nothing to do
        }

        if (!hasSession()) {
            qWarning() << "GlobalShortcutsPortal::bindShortcutsToCurrentSession: session was closed, aborting bind";
            return;
        }

        callBindShortcuts();
    });

    auto listReply = s_shortcutsInterface->ListShortcuts(sessionPath(), {{QLatin1String("handle_token"), listToken}});
    listReply.waitForFinished();
    if (listReply.isError()) {
        qWarning() << "GlobalShortcutsPortal::bindShortcutsToCurrentSession ListShortcuts failed:"
                   << listReply.error().message();
    }
}

void GlobalShortcutsPortal::callBindShortcuts()
{
    XdpShortcuts shortcuts = {
        {QLatin1String("autotype"), {{QStringLiteral("description"), tr("Trigger global Auto-Type")}}}};

    auto bindToken = portalRequest([this](uint bindResponse, const QVariantMap& bindResults) {
        if (bindResponse != 0) {
            qWarning() << "GlobalShortcutsPortal: BindShortcuts returned response" << bindResponse
                       << "(shortcut may still be active; known GNOME desktop portal bug in older versions)";
        } else {
            auto bound = qdbus_cast<XdpShortcuts>(bindResults.value("shortcuts"));
            if (!bound.isEmpty()) {
                for (const auto& [id, map] : bound) {
                    emit globalShortcutChanged(id, map.value(QStringLiteral("trigger_description")).toString());
                }
            }
        }

        // Setting this unconditionally because if the config is out-of-sync with system settings this'll ensure we
        // rebind them on startup
        config()->set(Config::GUI_XDPGlobalShortcutsConfigured, true);
    });

    auto reply =
        s_shortcutsInterface->BindShortcuts(sessionPath(), shortcuts, "", {{QLatin1String("handle_token"), bindToken}});
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "GlobalShortcutsPortal::callBindShortcuts BindShortcuts failed:" << reply.error().message();
    }
}
