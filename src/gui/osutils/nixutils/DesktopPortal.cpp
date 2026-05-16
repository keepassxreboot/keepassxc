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

#include "DesktopPortal.h"

#include "xdp_request.h"
#include "xdp_session.h"

#include <QDBusConnection>
#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>

namespace
{
    constexpr int s_portalRequestTimeoutMs = 30000;
} // namespace

DesktopPortal::DesktopPortal(QObject* parent)
    : QObject(parent)
{
}

DesktopPortal::~DesktopPortal()
{
    closeSession();
}

void DesktopPortal::closeSession()
{
    if (!m_session) {
        return;
    }

    disconnect(m_session, nullptr, this, nullptr);
    m_session->Close();
    m_session->deleteLater();
    m_session = nullptr;
}

bool DesktopPortal::hasSession() const
{
    return m_session != nullptr;
}

QDBusObjectPath DesktopPortal::sessionPath() const
{
    return QDBusObjectPath(m_session ? m_session->path() : QString());
}

bool DesktopPortal::isCurrentSession(const QDBusObjectPath& sessionHandle) const
{
    return m_session && sessionHandle.path() == m_session->path();
}

// Implements only 0.9+ requests where the path is known before making the call
QString DesktopPortal::portalRequest(const std::function<void(uint, const QVariantMap&)>& handler)
{
    static uint next;
    auto bus = QDBusConnection::sessionBus();
    auto token = QString("request_%1_%2").arg(++next).arg(QRandomGenerator::system()->generate());
    auto sender = bus.baseService().remove(0, 1).replace(".", "_");

    QStringList path;
    path << "" << "org" << "freedesktop" << "portal" << "desktop" << "request" << sender << token;

    auto req = new OrgFreedesktopPortalRequestInterface("org.freedesktop.portal.Desktop", path.join('/'), bus, this);
    connect(req, &OrgFreedesktopPortalRequestInterface::Response, req, handler);
    connect(req, &OrgFreedesktopPortalRequestInterface::Response, req, &QObject::deleteLater);

    auto timer = new QTimer(req);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, req, [req]() {
        qWarning() << "DesktopPortal::portalRequest: timed out waiting for portal response, closing request";
        req->Close();
        req->deleteLater();
    });
    connect(req, &OrgFreedesktopPortalRequestInterface::Response, timer, &QTimer::stop);
    timer->start(s_portalRequestTimeoutMs);

    return token;
}

QString DesktopPortal::prepareSession()
{
    closeSession();

    auto bus = QDBusConnection::sessionBus();
    auto token = "keepassxc_" + QString::number(QRandomGenerator::system()->generate());
    auto sender = bus.baseService().remove(0, 1).replace(".", "_");

    QStringList path;
    path << "" << "org" << "freedesktop" << "portal" << "desktop" << "session" << sender << token;

    auto* session =
        new OrgFreedesktopPortalSessionInterface("org.freedesktop.portal.Desktop", path.join('/'), bus, this);
    m_session = session;

    connect(session, &OrgFreedesktopPortalSessionInterface::Closed, this, [this, session](const QVariantMap& details) {
        if (m_session == session) {
            m_session = nullptr;
        }
        onSessionClosed(details);
    });
    connect(session, &OrgFreedesktopPortalSessionInterface::Closed, session, &QObject::deleteLater);

    return token;
}

void DesktopPortal::onSessionClosed(const QVariantMap& details)
{
    Q_UNUSED(details)
}
