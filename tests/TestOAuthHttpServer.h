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

#ifndef KEEPASSX_TESTOAUTHHTTPSERVER_H
#define KEEPASSX_TESTOAUTHHTTPSERVER_H

#include <QObject>

class TestOAuthHttpServer : public QObject
{
    Q_OBJECT

private slots:
    void testStartStop();
    void testStartFailsOnPortConflict();
    void testValidCodeCallback_emitsAuthCodeReceived();
    void testErrorCallback_emitsAuthError();
    void testStateMismatch_emits403AndAuthError();
    void testStateMatch_succeeds();
    void testNoCodeOrError_returns400();
    void testOversizedRequest_returns413();
    void testDoubleCode_secondIgnored();
    void testStateClearedAfterStop();
};

#endif // KEEPASSX_TESTOAUTHHTTPSERVER_H
