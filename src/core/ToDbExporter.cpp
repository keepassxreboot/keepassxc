/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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

#include "ToDbExporter.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"

Database* ToDbExporter::exportGroup(Group* group)
{
    Database* oldDb = group->database();
    Q_ASSERT(oldDb);

    Database* db = new Database();
    Group* clonedGroup = group->clone(Entry::CloneNewUuid | Entry::CloneIncludeHistory);
    clonedGroup->setParent(db->rootGroup());

    QSet<Uuid> customIcons = group->customIconsRecursive();
    db->metadata()->copyCustomIcons(customIcons, oldDb->metadata());

    db->copyAttributesFrom(oldDb);

    return db;
}
