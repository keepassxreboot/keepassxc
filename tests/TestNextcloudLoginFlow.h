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

#ifndef KEEPASSX_TESTNEXTCLOUDLOGINFLOW_H
#define KEEPASSX_TESTNEXTCLOUDLOGINFLOW_H

#include <QObject>

class TestNextcloudLoginFlow : public QObject
{
    Q_OBJECT

private slots:
    // Initiate success -> polling -> completion
    void testHappyPath_initiateThenPollSucceeds();

    // Phishing mitigation
    void testPhishing_loginUrlHostMismatch_failsBeforePolling();
    void testPhishing_pollEndpointHostMismatch_fails();
    void testPhishing_pollEndpointSchemeMismatch_fails();

    // Initiate failure paths
    void testInitiate_networkError_emitsFailed();
    void testInitiate_malformedJson_emitsFailed();
    void testInitiate_missingFields_emitsFailed();

    // Polling state machine
    void testPolling_keepsPollingOn404();
    void testPolling_keepsPollingOn3xx();
    void testPolling_hardFailureOn401();
    void testPolling_hardFailureOn500();
    void testPolling_timeoutFiresAfterPollTimeoutMs();

    // Cancel semantics
    void testCancel_inIdle_isNoop();
    void testCancel_duringInitiate_emitsCancelled();
    void testCancel_duringPolling_emitsCancelled();
    void testCancel_afterCompleted_doesNotReEmit();
    void testCancel_afterFailed_doesNotReEmit();

    // Cancel-previous on startLoginFlow
    void testStartLoginFlow_cancelsPreviousFlow();
};

#endif // KEEPASSX_TESTNEXTCLOUDLOGINFLOW_H
