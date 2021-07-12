/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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
#ifndef KEEPASSXC_SHAREEXPORT_H
#define KEEPASSXC_SHAREEXPORT_H

#include <QCoreApplication>

#include "keeshare/ShareObserver.h"

class Database;

class ShareExport
{
    Q_DECLARE_TR_FUNCTIONS(ShareExport)
public:
    static ShareObserver::Result
    intoContainer(const QString& resolvedPath, const KeeShareSettings::Reference& reference, const Group* group);

private:
    ShareExport() = delete;
};

#endif // KEEPASSXC_SHAREEXPORT_H
