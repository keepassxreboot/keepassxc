/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_TESTAUTOFILLUTILS_H
#define KEEPASSX_TESTAUTOFILLUTILS_H

#include <QObject>

class TestAutoFillUtils : public QObject
{
    Q_OBJECT

private slots:
    void testNormalizeHost();
    void testNormalizeHost_data();
    void testHostFromUrl();
    void testHostFromUrl_data();
    void testHostsMatch();
    void testHostsMatch_data();
};

#endif // KEEPASSX_TESTAUTOFILLUTILS_H
