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

#ifndef KEEPASSX_TESTREMOTESYNC_H
#define KEEPASSX_TESTREMOTESYNC_H

#include "config-keepassx.h"

#include <QObject>

class TestRemoteSync : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    // Factory + override seam
    void testFactoryDispatch_command();
#ifdef KPXC_FEATURE_NETWORK
    void testFactoryDispatch_dropbox();
#endif
    void testFactoryDispatch_unknown();
    void testFactoryOverride_routesThroughOverride();
    void testFactoryOverride_nullptrFallsThrough();

    // Default virtuals on the base class
    void testDefaultVirtuals();

    // CommandSyncProvider
    void testCommand_createParams_returnsCommandSyncParams();
    void testCommand_refreshAuth_isNoopSuccess();
    void testCommand_displayName();
    void testCommand_downloadDelegatesToRemoteHandler();
};

#endif // KEEPASSX_TESTREMOTESYNC_H
