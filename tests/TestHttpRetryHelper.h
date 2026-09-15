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

#ifndef KEEPASSX_TESTHTTPRETRYHELPER_H
#define KEEPASSX_TESTHTTPRETRYHELPER_H

#include <QObject>

class TestHttpRetryHelper : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testIsRetryable_table_data();
    void testIsRetryable_table();

    void testExecute_succeedsFirstAttempt();
    void testExecute_retriesOn500ThenSucceeds();
    void testExecute_exhaustsRetriesAndReturnsLastFailure();
    void testExecute_retryAfterCapTriggersImmediateFailure();
    void testExecute_abortFlagShortCircuits();
    void testExecute_abortFlagDuringDelayBreaksOut();
};

#endif // KEEPASSX_TESTHTTPRETRYHELPER_H
