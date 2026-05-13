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

#include "MockDropboxLoginFlow.h"

MockDropboxLoginFlow::MockDropboxLoginFlow(QObject* parent)
    : DropboxLoginFlow(parent)
{
}

void MockDropboxLoginFlow::setNextStartOutcome(StartOutcome outcome)
{
    m_nextStartOutcome = outcome;
}

void MockDropboxLoginFlow::setNextSubmitOutcome(SubmitOutcome outcome)
{
    m_nextSubmitOutcome = outcome;
}

void MockDropboxLoginFlow::setCannedTokens(const QString& accessToken, const QString& refreshToken, qint64 expiresAtMs)
{
    m_accessToken = accessToken;
    m_refreshToken = refreshToken;
    m_expiresAtMs = expiresAtMs;
}

void MockDropboxLoginFlow::setCannedManualVerifier(const QString& codeVerifier)
{
    m_codeVerifier = codeVerifier;
}

void MockDropboxLoginFlow::setCannedFailureReason(const QString& reason)
{
    m_failureReason = reason;
}

void MockDropboxLoginFlow::startAuthorization(const QString& /*appKey*/, int /*timeoutMs*/)
{
    ++m_startCount;
    emitStartOutcome();
}

void MockDropboxLoginFlow::submitManualCode(const QString& authCode, int /*timeoutMs*/)
{
    ++m_submitCount;
    m_lastSubmittedCode = authCode;
    emitSubmitOutcome();
}

void MockDropboxLoginFlow::cancel()
{
    ++m_cancelCount;
    // Cancel is the page-side's "stop the flow" entry; emit cancelled to
    // mirror the real flow's terminal transition. Tests that don't want the
    // cancellation signal can ignore it.
    emit authorizationCancelled();
}

void MockDropboxLoginFlow::emitStartOutcome()
{
    switch (m_nextStartOutcome) {
    case StartOutcome::ManualFallback:
        emit authorizationManualFallback(m_codeVerifier);
        return;
    case StartOutcome::Completed:
        emit authorizationCompleted(m_accessToken, m_refreshToken, m_expiresAtMs);
        return;
    case StartOutcome::Failed:
        emit authorizationFailed(m_failureReason);
        return;
    case StartOutcome::Cancelled:
        emit authorizationCancelled();
        return;
    }
}

void MockDropboxLoginFlow::emitSubmitOutcome()
{
    switch (m_nextSubmitOutcome) {
    case SubmitOutcome::Completed:
        emit authorizationCompleted(m_accessToken, m_refreshToken, m_expiresAtMs);
        return;
    case SubmitOutcome::Failed:
        emit authorizationFailed(m_failureReason);
        return;
    case SubmitOutcome::Cancelled:
        emit authorizationCancelled();
        return;
    }
}
