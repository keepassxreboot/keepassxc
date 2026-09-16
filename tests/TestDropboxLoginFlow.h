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

#ifndef KEEPASSX_TESTDROPBOXLOGINFLOW_H
#define KEEPASSX_TESTDROPBOXLOGINFLOW_H

#include <QObject>

class TestDropboxLoginFlow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // PKCE pure statics
    void testGenerateCodeVerifier_lengthAndCharset();
    void testGenerateCodeVerifier_isRandom();
    void testDeriveCodeChallenge_S256_isDeterministic();
    void testDeriveCodeChallenge_S256_matchesRFC7636Vector();
    void testDeriveCodeChallenge_differentInputsProduceDifferentOutputs();

    // State guards on submitManualCode
    void testSubmitManualCode_withoutManualFallbackState_emitsFailed();
    void testSubmitManualCode_emptyCode_emitsFailed();
    void testSubmitManualCode_whitespaceCode_emitsFailed();

    // startAuthorization guards
    void testStartAuthorization_emptyAppKey_emitsFailed();
    void testStartAuthorization_browserOpenerCalledWithCorrectQuery();
    void testStartAuthorization_manualFallback_emitsManualFallbackWithVerifier();

    // cancel semantics
    void testCancel_inIdle_isNoop();
    void testCancel_inAuthorizing_emitsAuthorizationCancelled();
    void testCancel_inManualFallback_emitsAuthorizationCancelled();
    void testCancel_afterTerminalCompletedOrFailed_doesNotReEmit();
};

#endif // KEEPASSX_TESTDROPBOXLOGINFLOW_H
