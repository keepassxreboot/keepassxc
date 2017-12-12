/*
 *  Copyright (C) 2016 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_KEEPASS2REPAIR_H
#define KEEPASSX_KEEPASS2REPAIR_H

#include <QCoreApplication>
#include <QIODevice>
#include <QPair>

#include "core/Database.h"
#include "keys/CompositeKey.h"

class KeePass2Repair
{
    Q_DECLARE_TR_FUNCTIONS(KeePass2Repair)

public:
    enum RepairResult
    {
        NothingTodo,
        UnableToOpen,
        RepairSuccess,
        RepairFailed
    };
    using RepairOutcome = QPair<RepairResult, Database*>;

    RepairOutcome repairDatabase(QIODevice* device, const CompositeKey& key);
    QString errorString() const;

private:
    QString m_errorStr;
};

#endif // KEEPASSX_KEEPASS2REPAIR_H
