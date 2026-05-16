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

#ifndef KEEPASSXC_DESKTOPPORTAL_H
#define KEEPASSXC_DESKTOPPORTAL_H

#include <QDBusObjectPath>
#include <QObject>
#include <QVariant>
#include <functional>

class OrgFreedesktopPortalSessionInterface;

class DesktopPortal : public QObject
{
    Q_OBJECT

public:
    void closeSession();
    bool hasSession() const;
    QDBusObjectPath sessionPath() const;
    bool isCurrentSession(const QDBusObjectPath& sessionHandle) const;

protected:
    explicit DesktopPortal(QObject* parent = nullptr);
    ~DesktopPortal() override;

    QString portalRequest(const std::function<void(uint, const QVariantMap&)>& handler);
    QString prepareSession();

    virtual void onSessionClosed(const QVariantMap& details);

private:
    OrgFreedesktopPortalSessionInterface* m_session = nullptr;
};

#endif // KEEPASSXC_DESKTOPPORTAL_H
