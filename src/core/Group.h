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

#include <QImage>
#include <QPixmap>
#include <QPixmapCache>
#include <QPointer>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/TimeInfo.h"
#include "core/Uuid.h"

class Group : public QObject
{
    Q_OBJECT

public:
    enum TriState { Inherit, Enable, Disable };

    struct GroupData
    {
        QString name;
        QString notes;
        int iconNumber;
        Uuid customIcon;
        TimeInfo timeInfo;
        bool isExpanded;
        QString defaultAutoTypeSequence;
        Group::TriState autoTypeEnabled;
        Group::TriState searchingEnabled;
    };

    Group();
    ~Group();

    static Group* createRecycleBin();

    Uuid uuid() const;
    QString name() const;
    QString notes() const;
    QImage icon() const;
    QPixmap iconPixmap() const;
    int iconNumber() const;
    Uuid iconUuid() const;
    TimeInfo timeInfo() const;
    bool isExpanded() const;
    QString defaultAutoTypeSequence() const;
    Group::TriState autoTypeEnabled() const;
    Group::TriState searchingEnabled() const;
    bool resolveSearchingEnabled() const;
    bool resolveAutoTypeEnabled() const;
    Entry* lastTopVisibleEntry() const;
    bool isExpired() const;

    static const int DefaultIconNumber;
    static const int RecycleBinIconNumber;

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
    void setExpires(bool value);
    void setExpiryTime(const QDateTime& dateTime);

    void setUpdateTimeinfo(bool value);

    Group* parentGroup();
    const Group* parentGroup() const;
    void setParent(Group* parent, int index = -1);

    Database* database();
    const Database* database() const;
    QList<Group*> children();
    const QList<Group*>& children() const;
    QList<Entry*> entries();
    const QList<Entry*>& entries() const;
    QList<Entry*> entriesRecursive(bool includeHistoryItems = false) const;
    QList<const Group*> groupsRecursive(bool includeSelf) const;
    QSet<Uuid> customIconsRecursive() const;
    /**
     * Creates a duplicate of this group including all child entries and groups.
     * The exceptions are that the returned group doesn't have a parent group
     * and all TimeInfo attributes are set to the current time.
     * Note that you need to copy the custom icons manually when inserting the
     * new group into another database.
     */
    Group* clone(Entry::CloneFlags entryFlags = Entry::CloneNewUuid | Entry::CloneResetTimeInfo) const;
    void copyDataFrom(const Group* other);

Q_SIGNALS:
    void dataChanged(Group* group);

    void aboutToAdd(Group* group, int index);
    void added();
    void aboutToRemove(Group* group);
    void removed();
    /**
     * Group moved within the database.
     */
    void aboutToMove(Group* group, Group* toGroup, int index);
    void moved();

    void entryAboutToAdd(Entry* entry);
    void entryAdded(Entry* entry);
    void entryAboutToRemove(Entry* entry);
    void entryRemoved(Entry* entry);

    void entryDataChanged(Entry* entry);

    void modified();

private:
    template <class P, class V> bool set(P& property, const V& value);

    void addEntry(Entry* entry);
    void removeEntry(Entry* entry);
    void setParent(Database* db);

    void recSetDatabase(Database* db);
    void cleanupParent();
    void recCreateDelObjects();
    void updateTimeinfo();

    QPointer<Database> m_db;
    Uuid m_uuid;
    GroupData m_data;
    QPointer<Entry> m_lastTopVisibleEntry;
    QList<Group*> m_children;
    QList<Entry*> m_entries;

    QPointer<Group> m_parent;
    mutable QPixmapCache::Key m_pixmapCacheKey;

    bool m_updateTimeinfo;

    friend void Database::setRootGroup(Group* group);
    friend Entry::~Entry();
    friend void Entry::setGroup(Group* group);
};

#endif // KEEPASSX_GROUP_H
