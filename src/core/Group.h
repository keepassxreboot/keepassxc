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

#ifndef KEEPASSX_GROUP_H
#define KEEPASSX_GROUP_H

#include <QtGui/QIcon>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/TimeInfo.h"
#include "core/Uuid.h"

class Group : public QObject
{
    Q_OBJECT

public:
    enum TriState { Inherit, Enable, Disable };

    Group();
    ~Group();
    Uuid uuid() const;
    QString name() const;
    QString notes() const;
    QIcon icon() const;
    int iconNumber() const;
    Uuid iconUuid() const;
    TimeInfo timeInfo() const;
    bool isExpanded() const;
    QString defaultAutoTypeSequence() const;
    Group::TriState autoTypeEnabled() const;
    Group::TriState searchingEnabed() const;
    Entry* lastTopVisibleEntry() const;

    void setUuid(const Uuid& uuid);
    void setName(const QString& name);
    void setNotes(const QString& notes);
    void setIcon(int iconNumber);
    void setIcon(const Uuid& uuid);
    void setTimeInfo(const TimeInfo& timeInfo);
    void setExpanded(bool expanded);
    void setDefaultAutoTypeSequence(const QString& sequence);
    void setAutoTypeEnabled(TriState enable);
    void setSearchingEnabled(TriState enable);
    void setLastTopVisibleEntry(Entry* entry);

    Group* parentGroup();
    const Group* parentGroup() const;
    void setParent(Group* parent, int index = -1);

    const Database* database() const;
    QList<Group*> children();
    const QList<Group*>& children() const;
    QList<Entry*> entries();
    const QList<Entry*>& entries() const;

Q_SIGNALS:
    void dataChanged(Group* group);

    void aboutToAdd(Group* group, int index);
    void added();
    void aboutToRemove(Group* group);
    void removed();

    void entryAboutToAdd(Entry* entry);
    void entryAdded();
    void entryAboutToRemove(Entry* entry);
    void entryRemoved();

    void entryDataChanged(Entry* entry);

private:
    void addEntry(Entry* entry);
    void removeEntry(Entry* entry);
    void setParent(Database* db);

    void recSetDatabase(Database* db);

    Database* m_db;
    Uuid m_uuid;
    QString m_name;
    QString m_notes;
    int m_iconNumber;
    Uuid m_customIcon;
    TimeInfo m_timeInfo;
    bool m_isExpanded;
    QString m_defaultAutoTypeSequence;
    TriState m_autoTypeEnabled;
    TriState m_searchingEnabled;
    Entry* m_lastTopVisibleEntry;
    QList<Group*> m_children;
    QList<Entry*> m_entries;

    Group* m_parent;

    friend void Database::setRootGroup(Group* group);
    friend void Entry::setGroup(Group *group);
};

#endif // KEEPASSX_GROUP_H
