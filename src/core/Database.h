/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_DATABASE_H
#define KEEPASSX_DATABASE_H

#include "Group.h"

#include <QtCore/QHash>
#include <QtGui/QIcon>

class Metadata;

struct DeletedObject
{
    Uuid uuid;
    QDateTime deletionTime;
};

class Database : public QObject
{
    Q_OBJECT

public:
    Database();
    Group* rootGroup();
    const Group* rootGroup() const;
    void setRootGroup(Group* group);
    Metadata* metadata();
    const Metadata* metadata() const;
    Entry* resolveEntry(const Uuid& uuid);
    Group* resolveGroup(const Uuid& uuid);
    QList<DeletedObject> deletedObjects();
    void addDeletedObject(const DeletedObject& delObj);

Q_SIGNALS:
    void groupDataChanged(Group* group);
    void groupAboutToAdd(Group* group, int index);
    void groupAdded();
    void groupAboutToRemove(Group* group);
    void groupRemoved();

private:
    Entry* recFindEntry(const Uuid& uuid, Group* group);
    Group* recFindGroup(const Uuid& uuid, Group* group);

    Metadata* m_metadata;
    Group* m_rootGroup;
    QList<DeletedObject> m_deletedObjects;
};

#endif // KEEPASSX_DATABASE_H
