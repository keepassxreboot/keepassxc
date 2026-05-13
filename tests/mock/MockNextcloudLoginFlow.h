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

#ifndef KEEPASSXC_MOCKNEXTCLOUDLOGINFLOW_H
#define KEEPASSXC_MOCKNEXTCLOUDLOGINFLOW_H

#include "remotesync/NextcloudLoginFlow.h"

// Test double for NextcloudLoginFlow: overrides startLoginFlow / cancel so the
// page can drive its auth state machine without real Login Flow v2 POST /
// browser-open / 5-second polling machinery. Tests configure the outcome of
// the next start call ahead of time; the mock emits the matching terminal
// signal synchronously when the page invokes the virtual.
//
// Mirrors MockDropboxLoginFlow's shape -- StartOutcome enum, set/canned
// helpers, call counters -- minus the manual-fallback path (Nextcloud Login
// Flow v2 has no analogue of Dropbox's PKCE manual-code fallback).
class MockNextcloudLoginFlow : public NextcloudLoginFlow
{
    Q_OBJECT

public:
    enum class StartOutcome
    {
        Completed,
        Failed,
        Cancelled,
    };

    explicit MockNextcloudLoginFlow(QObject* parent = nullptr);
    ~MockNextcloudLoginFlow() override = default;

    void setNextStartOutcome(StartOutcome outcome);
    void setCannedCreds(const QString& loginName, const QString& appPassword);
    void setCannedFailureReason(const QString& reason);

    void startLoginFlow(const QString& serverBaseUrl) override;
    void cancel() override;

    int startCount() const { return m_startCount; }
    int cancelCount() const { return m_cancelCount; }
    QString lastServerBaseUrl() const { return m_lastServerBaseUrl; }

private:
    void emitStartOutcome();

    StartOutcome m_nextStartOutcome = StartOutcome::Completed;

    QString m_loginName;
    QString m_appPassword;
    QString m_failureReason;

    int m_startCount = 0;
    int m_cancelCount = 0;
    QString m_lastServerBaseUrl;
};

#endif // KEEPASSXC_MOCKNEXTCLOUDLOGINFLOW_H
