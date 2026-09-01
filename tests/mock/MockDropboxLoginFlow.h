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

#ifndef KEEPASSXC_MOCKDROPBOXLOGINFLOW_H
#define KEEPASSXC_MOCKDROPBOXLOGINFLOW_H

#include "remotesync/DropboxLoginFlow.h"

// Test double for DropboxLoginFlow: overrides the three virtuals
// (startAuthorization / submitManualCode / cancel) so the page can drive its
// auth state machine without real PKCE / browser-open / OAuthHttpServer
// machinery. Tests configure the outcome of the next start/submit call ahead
// of time; the mock emits the matching terminal signal synchronously when
// the page invokes the virtual.
class MockDropboxLoginFlow : public DropboxLoginFlow
{
    Q_OBJECT

public:
    enum class StartOutcome
    {
        ManualFallback,
        Completed,
        Failed,
        Cancelled,
    };

    enum class SubmitOutcome
    {
        Completed,
        Failed,
        Cancelled,
    };

    explicit MockDropboxLoginFlow(QObject* parent = nullptr);
    ~MockDropboxLoginFlow() override = default;

    void setNextStartOutcome(StartOutcome outcome);
    void setNextSubmitOutcome(SubmitOutcome outcome);
    void setCannedTokens(const QString& accessToken, const QString& refreshToken, qint64 expiresAtMs);
    void setCannedManualVerifier(const QString& codeVerifier);
    void setCannedFailureReason(const QString& reason);

    void startAuthorization(const QString& appKey, int timeoutMs) override;
    void submitManualCode(const QString& authCode, int timeoutMs) override;
    void cancel() override;

    int startCount() const { return m_startCount; }
    int submitCount() const { return m_submitCount; }
    int cancelCount() const { return m_cancelCount; }
    QString lastSubmittedCode() const { return m_lastSubmittedCode; }

private:
    void emitStartOutcome();
    void emitSubmitOutcome();

    StartOutcome m_nextStartOutcome = StartOutcome::Completed;
    SubmitOutcome m_nextSubmitOutcome = SubmitOutcome::Completed;

    QString m_accessToken;
    QString m_refreshToken;
    qint64 m_expiresAtMs = 0;
    QString m_codeVerifier;
    QString m_failureReason;

    int m_startCount = 0;
    int m_submitCount = 0;
    int m_cancelCount = 0;
    QString m_lastSubmittedCode;
};

#endif // KEEPASSXC_MOCKDROPBOXLOGINFLOW_H
