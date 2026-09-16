/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#include "MockNextcloudLoginFlow.h"

MockNextcloudLoginFlow::MockNextcloudLoginFlow(QObject* parent)
    : NextcloudLoginFlow(parent)
{
}

void MockNextcloudLoginFlow::setNextStartOutcome(StartOutcome outcome)
{
    m_nextStartOutcome = outcome;
}

void MockNextcloudLoginFlow::setCannedCreds(const QString& loginName, const QString& appPassword)
{
    m_loginName = loginName;
    m_appPassword = appPassword;
}

void MockNextcloudLoginFlow::setCannedFailureReason(const QString& reason)
{
    m_failureReason = reason;
}

void MockNextcloudLoginFlow::startLoginFlow(const QString& serverBaseUrl)
{
    ++m_startCount;
    m_lastServerBaseUrl = serverBaseUrl;
    emitStartOutcome();
}

void MockNextcloudLoginFlow::cancel()
{
    ++m_cancelCount;
    // Cancel is the page-side's "stop the flow" entry; emit cancelled to
    // mirror the real flow's terminal transition. Tests that don't want the
    // cancellation signal can ignore it.
    emit loginCancelled();
}

void MockNextcloudLoginFlow::emitStartOutcome()
{
    switch (m_nextStartOutcome) {
    case StartOutcome::Completed:
        emit loginCompleted(m_loginName, m_appPassword);
        return;
    case StartOutcome::Failed:
        emit loginFailed(m_failureReason);
        return;
    case StartOutcome::Cancelled:
        emit loginCancelled();
        return;
    }
}
