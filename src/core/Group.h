/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_GROUP_H
#define KEEPASSX_GROUP_H

#include <QImage>
#include <QPixmap>
#include <QPixmapCache>
#include <QPointer>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/CustomData.h"
#include "core/TimeInfo.h"
#include "core/Uuid.h"

class Group : public QObject
{
    Q_OBJECT

public:
    enum TriState { Inherit, Enable, Disable };
    enum MergeMode { ModeInherit, KeepBoth, KeepNewer, KeepExisting };

    enum CloneFlag {
        CloneNoFlags        = 0,
        CloneNewUuid        = 1,  // generate a random uuid for the clone
        CloneResetTimeInfo  = 2,  // set all TimeInfo attributes to the current time
        CloneIncludeEntries = 4,  // clone the group entries
    };
    Q_DECLARE_FLAGS(CloneFlags, CloneFlag)

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
        Group::MergeMode mergeMode;
    };

    Group();
    ~Group();

    static Group* createRecycleBin();

    Uuid uuid() const;
    QString name() const;
    QString notes() const;
    QImage icon() const;
    QPixmap iconPixmap() const;
    QPixmap iconScaledPixmap() const;
    int iconNumber() const;
    Uuid iconUuid() const;
    TimeInfo timeInfo() const;
    bool isExpanded() const;
    QString defaultAutoTypeSequence() const;
    QString effectiveAutoTypeSequence() const;
    Group::TriState autoTypeEnabled() const;
    Group::TriState searchingEnabled() const;
    Group::MergeMode mergeMode() const;
    bool resolveSearchingEnabled() const;
    bool resolveAutoTypeEnabled() const;
    Entry* lastTopVisibleEntry() const;
    bool isExpired() const;
    CustomData* customData();
    const CustomData* customData() const;

    static const int DefaultIconNumber;
    static const int RecycleBinIconNumber;
    static CloneFlags DefaultCloneFlags;
    static Entry::CloneFlags DefaultEntryCloneFlags;
    static const QString RootAutoTypeSequence;

    Group* findChildByName(const QString& name);
    Group* findChildByUuid(const Uuid& uuid);
    Entry* findEntry(QString entryId);
    Entry* findEntryByUuid(const Uuid& uuid);
    Entry* findEntryByPath(QString entryPath, QString basePath = QString(""));
    Group* findGroupByPath(QString groupPath, QString basePath = QString("/"));
    QStringList locate(QString locateTerm, QString currentPath = QString("/"));
    Entry* addEntryWithPath(QString entryPath);
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
    void setMergeMode(MergeMode newMode);

    void setUpdateTimeinfo(bool value);

    Group* parentGroup();
    const Group* parentGroup() const;
    void setParent(Group* parent, int index = -1);
    QStringList hierarchy() const;

    Database* database();
    const Database* database() const;
    QList<Group*> children();
    const QList<Group*>& children() const;
    QList<Entry*> entries();
    const QList<Entry*>& entries() const;
    QList<Entry*> entriesRecursive(bool includeHistoryItems = false) const;
    QList<const Group*> groupsRecursive(bool includeSelf) const;
    QList<Group*> groupsRecursive(bool includeSelf);
    QSet<Uuid> customIconsRecursive() const;
    /**
     * Creates a duplicate of this group.
     * Note that you need to copy the custom icons manually when inserting the
     * new group into another database.
     */
    Group* clone(Entry::CloneFlags entryFlags = DefaultEntryCloneFlags,
                 CloneFlags groupFlags = DefaultCloneFlags) const;

    void copyDataFrom(const Group* other);
    void merge(const Group* other);
    QString print(bool recursive = false, int depth = 0);

signals:
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

private slots:
    void updateTimeinfo();

private:
    template <class P, class V> bool set(P& property, const V& value);

    void addEntry(Entry* entry);
    void removeEntry(Entry* entry);
    void setParent(Database* db);
    void markOlderEntry(Entry* entry);
    void resolveEntryConflict(Entry* existingEntry, Entry* otherEntry);
    void resolveGroupConflict(Group* existingGroup, Group* otherGroup);

    void recSetDatabase(Database* db);
    void cleanupParent();
    void recCreateDelObjects();

    QPointer<Database> m_db;
    Uuid m_uuid;
    GroupData m_data;
    QPointer<Entry> m_lastTopVisibleEntry;
    QList<Group*> m_children;
    QList<Entry*> m_entries;

    QPointer<CustomData> m_customData;

    QPointer<Group> m_parent;

    bool m_updateTimeinfo;

    friend void Database::setRootGroup(Group* group);
    friend Entry::~Entry();
    friend void Entry::setGroup(Group* group);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Group::CloneFlags)

#endif // KEEPASSX_GROUP_H
