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

#ifndef KEEPASSX_TESTSYNCENGINE_H
#define KEEPASSX_TESTSYNCENGINE_H

#include <QObject>

class TestSyncEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // State machine basics
    void testInitialState_isIdle();
    void testStartSync_whenAlreadyRunning_returnsFalseAndEmitsError();
    void testHappyPath_runsToCompletion();
    void testFirstSync_skipsMerge();

    // Error paths
    void testRefreshAuthFails_emitsSyncFinishedFalseAndSetsLastErrorKind();
    void testDownloadFails_emitsSyncFinishedFalseAndSetsLastErrorKind();
    void testUploadFails_emitsSyncFinishedFalseAndSetsLastErrorKind();
    void testSaveFails_emitsSyncFinishedFalse();

    // Cancel semantics
    void testCancel_betweenRefreshAuthAndDownload();
    void testCancel_inIdle_isNoop();

    // applyRefreshedTokens
    void testApplyRefreshedTokens_failureSurfacesAsAuthError();
    void testRefreshedTokenData_signalFiresOnRefreshSuccess();

    // remoteDbNeedsKey (key mismatch)
    void testRemoteDbNeedsKey_whenLocalAndRemoteKeysMismatch();

    // syncPreviousKey integration
    void testClearSyncPreviousKey_onSuccessfulUpload();

    // Temp file cleanup
    void testTempFileRemovedOnSuccess();
    void testTempFileRemovedOnFailure();

    // State change signal sequence
    void testStateChangeSequence_happyPath();
    void testStateChangeSequence_withMerge();
};

#endif // KEEPASSX_TESTSYNCENGINE_H
