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

#include "BackgroundPortal.h"

#include "xdp_background.h"

#include <QDebug>
#include <QRandomGenerator>

Q_GLOBAL_STATIC_WITH_ARGS(OrgFreedesktopPortalBackgroundInterface,
                          s_backgroundInterface,
                          ("org.freedesktop.portal.Desktop",
                           "/org/freedesktop/portal/desktop",
                           QDBusConnection::sessionBus()));

BackgroundPortal::BackgroundPortal(QObject* parent)
    : DesktopPortal(parent)
{
}

bool BackgroundPortal::isAvailable() const
{
    return s_backgroundInterface->isValid();
}

void BackgroundPortal::requestBackground(bool autostart)
{
    if (!isAvailable()) {
        qWarning() << "Background portal is not available";
        return;
    }

    auto token = portalRequest([](uint response, const QVariantMap&) {
        if (response != 0) {
            qWarning() << "Background portal request failed with response:" << response;
        }
    });

    auto reply =
        s_backgroundInterface->RequestBackground("",
                                                 QVariantMap{
                                                     {QLatin1String("autostart"), autostart},
                                                     {QLatin1String("reason"), tr("Launch KeePassXC at startup")},
                                                     {QLatin1String("handle_token"), token},
                                                 });
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "Failed to call RequestBackground:" << reply.error().message();
    }
}
