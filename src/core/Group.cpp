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

#include "Group.h"

#include "core/Config.h"
#include "core/DatabaseIcons.h"
#include "core/Global.h"
#include "core/Metadata.h"

const int Group::DefaultIconNumber = 48;
const int Group::RecycleBinIconNumber = 43;
const QString Group::RootAutoTypeSequence = "{USERNAME}{TAB}{PASSWORD}{ENTER}";

Group::CloneFlags Group::DefaultCloneFlags = static_cast<Group::CloneFlags>(
    Group::CloneNewUuid | Group::CloneResetTimeInfo | Group::CloneIncludeEntries);
Entry::CloneFlags Group::DefaultEntryCloneFlags = static_cast<Entry::CloneFlags>(
    Entry::CloneNewUuid | Entry::CloneResetTimeInfo);

Group::Group()
    : m_customData(new CustomData(this))
    , m_updateTimeinfo(true)
{
    m_data.iconNumber = DefaultIconNumber;
    m_data.isExpanded = true;
    m_data.autoTypeEnabled = Inherit;
    m_data.searchingEnabled = Inherit;
    m_data.mergeMode = ModeInherit;

    connect(m_customData, SIGNAL(modified()), this, SIGNAL(modified()));
    connect(this, SIGNAL(modified()), SLOT(updateTimeinfo()));
}

Group::~Group()
{
    // Destroy entries and children manually so DeletedObjects can be added
    // to database.
    const QList<Entry*> entries = m_entries;
    for (Entry* entry : entries) {
        delete entry;
    }

    const QList<Group*> children = m_children;
    for (Group* group : children) {
        delete group;
    }

    if (m_db && m_parent) {
        DeletedObject delGroup;
        delGroup.deletionTime = QDateTime::currentDateTimeUtc();
        delGroup.uuid = m_uuid;
        m_db->addDeletedObject(delGroup);
    }

    cleanupParent();
}

Group* Group::createRecycleBin()
{
    Group* recycleBin = new Group();
    recycleBin->setUuid(Uuid::random());
    recycleBin->setName(tr("Recycle Bin"));
    recycleBin->setIcon(RecycleBinIconNumber);
    recycleBin->setSearchingEnabled(Group::Disable);
    recycleBin->setAutoTypeEnabled(Group::Disable);
    return recycleBin;
}

template <class P, class V> inline bool Group::set(P& property, const V& value) {
    if (property != value) {
        property = value;
        emit modified();
        return true;
    } else {
        return false;
    }
}

void Group::updateTimeinfo()
{
    if (m_updateTimeinfo) {
        m_data.timeInfo.setLastModificationTime(QDateTime::currentDateTimeUtc());
        m_data.timeInfo.setLastAccessTime(QDateTime::currentDateTimeUtc());
    }
}

void Group::setUpdateTimeinfo(bool value)
{
    m_updateTimeinfo = value;
}

Uuid Group::uuid() const
{
    return m_uuid;
}

QString Group::name() const
{
    return m_data.name;
}

QString Group::notes() const
{
    return m_data.notes;
}

QImage Group::icon() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->icon(m_data.iconNumber);
    }
    else {
        Q_ASSERT(m_db);

        if (m_db) {
            return m_db->metadata()->customIcon(m_data.customIcon);
        }
        else {
            return QImage();
        }
    }
}

QPixmap Group::iconPixmap() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    }
    else {
        Q_ASSERT(m_db);

        if (m_db) {
            return m_db->metadata()->customIconPixmap(m_data.customIcon);
        }
        else {
            return QPixmap();
        }
    }
}

QPixmap Group::iconScaledPixmap() const
{
    if (m_data.customIcon.isNull()) {
        // built-in icons are 16x16 so don't need to be scaled
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    }
    else {
        Q_ASSERT(m_db);

        if (m_db) {
            return m_db->metadata()->customIconScaledPixmap(m_data.customIcon);
        }
        else {
            return QPixmap();
        }
    }
}

int Group::iconNumber() const
{
    return m_data.iconNumber;
}

Uuid Group::iconUuid() const
{
    return m_data.customIcon;
}

TimeInfo Group::timeInfo() const
{
    return m_data.timeInfo;
}

bool Group::isExpanded() const
{
    return m_data.isExpanded;
}

QString Group::defaultAutoTypeSequence() const
{
    return m_data.defaultAutoTypeSequence;
}

/**
 * Determine the effective sequence that will be injected
 * This function return an empty string if the current group or any parent has autotype disabled
 */
QString Group::effectiveAutoTypeSequence() const
{
    QString sequence;

    const Group* group = this;
    do {
        if (group->autoTypeEnabled() == Group::Disable) {
            return QString();
        }

        sequence = group->defaultAutoTypeSequence();
        group = group->parentGroup();
    } while (group && sequence.isEmpty());

    if (sequence.isEmpty()) {
        sequence = RootAutoTypeSequence;
    }

    return sequence;
}

Group::TriState Group::autoTypeEnabled() const
{
    return m_data.autoTypeEnabled;
}

Group::TriState Group::searchingEnabled() const
{
    return m_data.searchingEnabled;
}

Group::MergeMode Group::mergeMode() const
{
    if (m_data.mergeMode == Group::MergeMode::ModeInherit) {
        if (m_parent) {
            return m_parent->mergeMode();
        } else {
            return Group::MergeMode::KeepNewer; // fallback
        }
    } else {
        return m_data.mergeMode;
    }
}

Entry* Group::lastTopVisibleEntry() const
{
    return m_lastTopVisibleEntry;
}

bool Group::isExpired() const
{
    return m_data.timeInfo.expires() && m_data.timeInfo.expiryTime() < QDateTime::currentDateTimeUtc();
}

CustomData* Group::customData()
{
    return m_customData;
}

const CustomData* Group::customData() const
{
    return m_customData;
}

void Group::setUuid(const Uuid& uuid)
{
    set(m_uuid, uuid);
}

void Group::setName(const QString& name)
{
    if (set(m_data.name, name)) {
        emit dataChanged(this);
    }
}

void Group::setNotes(const QString& notes)
{
    set(m_data.notes, notes);
}

void Group::setIcon(int iconNumber)
{
    Q_ASSERT(iconNumber >= 0);

    if (m_data.iconNumber != iconNumber || !m_data.customIcon.isNull()) {
        m_data.iconNumber = iconNumber;
        m_data.customIcon = Uuid();
        emit modified();
        emit dataChanged(this);
    }
}

void Group::setIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    if (m_data.customIcon != uuid) {
        m_data.customIcon = uuid;
        m_data.iconNumber = 0;
        emit modified();
        emit dataChanged(this);
    }
}

void Group::setTimeInfo(const TimeInfo& timeInfo)
{
    m_data.timeInfo = timeInfo;
}

void Group::setExpanded(bool expanded)
{
    if (m_data.isExpanded != expanded) {
        m_data.isExpanded = expanded;
        if (config()->get("IgnoreGroupExpansion").toBool()) {
            updateTimeinfo();
            return;
        }
        emit modified();
    }
}

void Group::setDefaultAutoTypeSequence(const QString& sequence)
{
    set(m_data.defaultAutoTypeSequence, sequence);
}

void Group::setAutoTypeEnabled(TriState enable)
{
    set(m_data.autoTypeEnabled, enable);
}

void Group::setSearchingEnabled(TriState enable)
{
    set(m_data.searchingEnabled, enable);
}

void Group::setLastTopVisibleEntry(Entry* entry)
{
    set(m_lastTopVisibleEntry, entry);
}

void Group::setExpires(bool value)
{
    if (m_data.timeInfo.expires() != value) {
        m_data.timeInfo.setExpires(value);
        emit modified();
    }
}

void Group::setExpiryTime(const QDateTime& dateTime)
{
    if (m_data.timeInfo.expiryTime() != dateTime) {
        m_data.timeInfo.setExpiryTime(dateTime);
        emit modified();
    }
}

void Group::setMergeMode(MergeMode newMode)
{
    set(m_data.mergeMode, newMode);
}

Group* Group::parentGroup()
{
    return m_parent;
}

const Group* Group::parentGroup() const
{
    return m_parent;
}

void Group::setParent(Group* parent, int index)
{
    Q_ASSERT(parent);
    Q_ASSERT(index >= -1 && index <= parent->children().size());
    // setting a new parent for root groups is not allowed
    Q_ASSERT(!m_db || (m_db->rootGroup() != this));

    bool moveWithinDatabase = (m_db && m_db == parent->m_db);

    if (index == -1) {
        index = parent->children().size();

        if (parentGroup() == parent) {
            index--;
        }
    }

    if (m_parent == parent && parent->children().indexOf(this) == index) {
        return;
    }

    if (!moveWithinDatabase) {
        cleanupParent();
        m_parent = parent;
        if (m_db) {
            recCreateDelObjects();

            // copy custom icon to the new database
            if (!iconUuid().isNull() && parent->m_db
                    && m_db->metadata()->containsCustomIcon(iconUuid())
                    && !parent->m_db->metadata()->containsCustomIcon(iconUuid())) {
                parent->m_db->metadata()->addCustomIcon(iconUuid(), icon());
            }
        }
        if (m_db != parent->m_db) {
            recSetDatabase(parent->m_db);
        }
        QObject::setParent(parent);
        emit aboutToAdd(this, index);
        Q_ASSERT(index <= parent->m_children.size());
        parent->m_children.insert(index, this);
    }
    else {
        emit aboutToMove(this, parent, index);
        m_parent->m_children.removeAll(this);
        m_parent = parent;
        QObject::setParent(parent);
        Q_ASSERT(index <= parent->m_children.size());
        parent->m_children.insert(index, this);
    }

    if (m_updateTimeinfo) {
        m_data.timeInfo.setLocationChanged(QDateTime::currentDateTimeUtc());
    }

    emit modified();

    if (!moveWithinDatabase) {
        emit added();
    }
    else {
        emit moved();
    }
}

void Group::setParent(Database* db)
{
    Q_ASSERT(db);
    Q_ASSERT(db->rootGroup() == this);

    cleanupParent();

    m_parent = nullptr;
    recSetDatabase(db);

    QObject::setParent(db);
}

QStringList Group::hierarchy() const
{
    QStringList hierarchy;
    const Group* group = this;
    const Group* parent = m_parent;
    hierarchy.prepend(group->name());
    
    while (parent) {
        group = group->parentGroup();
        parent = group->parentGroup();

        hierarchy.prepend(group->name());
    }

    return hierarchy;
}

Database* Group::database()
{
    return m_db;
}

const Database* Group::database() const
{
    return m_db;
}

QList<Group*> Group::children()
{
    return m_children;
}

const QList<Group*>& Group::children() const
{
    return m_children;
}

QList<Entry*> Group::entries()
{
    return m_entries;
}

const QList<Entry*>& Group::entries() const
{
    return m_entries;
}

QList<Entry*> Group::entriesRecursive(bool includeHistoryItems) const
{
    QList<Entry*> entryList;

    entryList.append(m_entries);

    if (includeHistoryItems) {
        for (Entry* entry : m_entries) {
            entryList.append(entry->historyItems());
        }
    }

    for (Group* group : m_children) {
        entryList.append(group->entriesRecursive(includeHistoryItems));
    }

    return entryList;
}

Entry* Group::findEntry(QString entryId)
{
    Q_ASSERT(!entryId.isNull());

    Entry* entry;
    if (Uuid::isUuid(entryId)) {
        entry = findEntryByUuid(Uuid::fromHex(entryId));
        if (entry) {
            return entry;
        }
    }

    entry = findEntryByPath(entryId);
    if (entry) {
        return entry;
    }

    for (Entry* entry : entriesRecursive(false)) {
        if (entry->title() == entryId) {
            return entry;
        }
    }

    return nullptr;
}

Entry* Group::findEntryByUuid(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());
    for (Entry* entry : entriesRecursive(false)) {
        if (entry->uuid() == uuid) {
            return entry;
        }
    }

    return nullptr;
}

Entry* Group::findEntryByPath(QString entryPath, QString basePath)
{

    Q_ASSERT(!entryPath.isNull());

    for (Entry* entry : asConst(m_entries)) {
        QString currentEntryPath = basePath + entry->title();
        if (entryPath == currentEntryPath || entryPath == QString("/" + currentEntryPath)) {
            return entry;
        }
    }

    for (Group* group : asConst(m_children)) {
        Entry* entry = group->findEntryByPath(entryPath, basePath + group->name() + QString("/"));
        if (entry != nullptr) {
            return entry;
        }
    }

    return nullptr;
}

Group* Group::findGroupByPath(QString groupPath, QString basePath)
{

    Q_ASSERT(!groupPath.isNull());

    QStringList possiblePaths;
    possiblePaths << groupPath;
    if (!groupPath.startsWith("/")) {
        possiblePaths << QString("/" + groupPath);
    }
    if (!groupPath.endsWith("/")) {
        possiblePaths << QString(groupPath + "/");
    }
    if (!groupPath.endsWith("/") && !groupPath.endsWith("/")) {
        possiblePaths << QString("/" + groupPath + "/");
    }

    if (possiblePaths.contains(basePath)) {
        return this;
    }

    for (Group* innerGroup : children()) {
        QString innerBasePath = basePath + innerGroup->name() + "/";
        Group* group = innerGroup->findGroupByPath(groupPath, innerBasePath);
        if (group != nullptr) {
            return group;
        }
    }

    return nullptr;
}

QString Group::print(bool recursive, int depth)
{

    QString response;
    QString indentation = QString("  ").repeated(depth);

    if (entries().isEmpty() && children().isEmpty()) {
        response += indentation + "[empty]\n";
        return response;
    }

    for (Entry* entry : entries()) {
        response += indentation + entry->title() + "\n";
    }

    for (Group* innerGroup : children()) {
        response += indentation + innerGroup->name() + "/\n";
        if (recursive) {
            response += innerGroup->print(recursive, depth + 1);
        }
    }

    return response;
}

QList<const Group*> Group::groupsRecursive(bool includeSelf) const
{
    QList<const Group*> groupList;
    if (includeSelf) {
        groupList.append(this);
    }

    for (const Group* group : m_children) {
        groupList.append(group->groupsRecursive(true));
    }

    return groupList;
}

QList<Group*> Group::groupsRecursive(bool includeSelf)
{
    QList<Group*> groupList;
    if (includeSelf) {
        groupList.append(this);
    }

    for (Group* group : asConst(m_children)) {
        groupList.append(group->groupsRecursive(true));
    }

    return groupList;
}

QSet<Uuid> Group::customIconsRecursive() const
{
    QSet<Uuid> result;

    if (!iconUuid().isNull()) {
        result.insert(iconUuid());
    }

    const QList<Entry*> entryList = entriesRecursive(true);
    for (Entry* entry : entryList) {
        if (!entry->iconUuid().isNull()) {
            result.insert(entry->iconUuid());
        }
    }

    for (Group* group : m_children) {
        result.unite(group->customIconsRecursive());
    }

    return result;
}

void Group::merge(const Group* other)
{

    Group* rootGroup = this;
    while (rootGroup->parentGroup()) {
        rootGroup = rootGroup->parentGroup();
    }

    // merge entries
    const QList<Entry*> dbEntries = other->entries();
    for (Entry* entry : dbEntries) {

        Entry* existingEntry = rootGroup->findEntryByUuid(entry->uuid());

        if (!existingEntry) {
            // This entry does not exist at all. Create it.
            qDebug("New entry %s detected. Creating it.", qPrintable(entry->title()));
            entry->clone(Entry::CloneIncludeHistory)->setGroup(this);
        } else {
            // Entry is already present in the database. Update it.
            bool locationChanged = existingEntry->timeInfo().locationChanged() < entry->timeInfo().locationChanged();
            if (locationChanged && existingEntry->group() != this) {
                existingEntry->setGroup(this);
                qDebug("Location changed for entry %s. Updating it", qPrintable(existingEntry->title()));
            }
            resolveEntryConflict(existingEntry, entry);
        }
    }

    // merge groups recursively
    const QList<Group*> dbChildren = other->children();
    for (Group* group : dbChildren) {

        Group* existingGroup = rootGroup->findChildByUuid(group->uuid());

        if (!existingGroup) {
            qDebug("New group %s detected. Creating it.", qPrintable(group->name()));
            Group* newGroup = group->clone(Entry::CloneNoFlags, Group::CloneNoFlags);
            newGroup->setParent(this);
            newGroup->merge(group);
        } else {
            bool locationChanged = existingGroup->timeInfo().locationChanged() < group->timeInfo().locationChanged();
            if (locationChanged && existingGroup->parent() != this) {
                existingGroup->setParent(this);
                qDebug("Location changed for group %s. Updating it", qPrintable(existingGroup->name()));
            }
            resolveGroupConflict(existingGroup, group);
            existingGroup->merge(group);
        }

    }

    emit modified();
}

Group* Group::findChildByUuid(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());
    for (Group* group : groupsRecursive(true)) {
        if (group->uuid() == uuid) {
            return group;
        }
    }

    return nullptr;
}

Group* Group::findChildByName(const QString& name)
{
    for (Group* group : asConst(m_children)) {
        if (group->name() == name) {
            return group;
        }
    }

    return nullptr;
}

Group* Group::clone(Entry::CloneFlags entryFlags, Group::CloneFlags groupFlags) const
{
    Group* clonedGroup = new Group();

    clonedGroup->setUpdateTimeinfo(false);

    if (groupFlags & Group::CloneNewUuid) {
        clonedGroup->setUuid(Uuid::random());
    } else {
        clonedGroup->setUuid(this->uuid());
    }

    clonedGroup->m_data = m_data;
    clonedGroup->m_customData->copyDataFrom(m_customData);

    if (groupFlags & Group::CloneIncludeEntries) {
        const QList<Entry*> entryList = entries();
        for (Entry* entry : entryList) {
            Entry* clonedEntry = entry->clone(entryFlags);
            clonedEntry->setGroup(clonedGroup);
        }

        const QList<Group*> childrenGroups = children();
        for (Group* groupChild : childrenGroups) {
            Group* clonedGroupChild = groupChild->clone(entryFlags, groupFlags);
            clonedGroupChild->setParent(clonedGroup);
        }
    }

    clonedGroup->setUpdateTimeinfo(true);
    if (groupFlags & Group::CloneResetTimeInfo) {

        QDateTime now = QDateTime::currentDateTimeUtc();
        clonedGroup->m_data.timeInfo.setCreationTime(now);
        clonedGroup->m_data.timeInfo.setLastModificationTime(now);
        clonedGroup->m_data.timeInfo.setLastAccessTime(now);
        clonedGroup->m_data.timeInfo.setLocationChanged(now);
    }

    return clonedGroup;
}

void Group::copyDataFrom(const Group* other)
{
    m_data = other->m_data;
    m_customData->copyDataFrom(other->m_customData);
    m_lastTopVisibleEntry = other->m_lastTopVisibleEntry;
}

void Group::addEntry(Entry* entry)
{
    Q_ASSERT(entry);
    Q_ASSERT(!m_entries.contains(entry));

    emit entryAboutToAdd(entry);

    m_entries << entry;
    connect(entry, SIGNAL(dataChanged(Entry*)), SIGNAL(entryDataChanged(Entry*)));
    if (m_db) {
        connect(entry, SIGNAL(modified()), m_db, SIGNAL(modifiedImmediate()));
    }

    emit modified();
    emit entryAdded(entry);
}

void Group::removeEntry(Entry* entry)
{
    Q_ASSERT(m_entries.contains(entry));

    emit entryAboutToRemove(entry);

    entry->disconnect(this);
    if (m_db) {
        entry->disconnect(m_db);
    }
    m_entries.removeAll(entry);
    emit modified();
    emit entryRemoved(entry);
}

void Group::recSetDatabase(Database* db)
{
    if (m_db) {
        disconnect(SIGNAL(dataChanged(Group*)), m_db);
        disconnect(SIGNAL(aboutToRemove(Group*)), m_db);
        disconnect(SIGNAL(removed()), m_db);
        disconnect(SIGNAL(aboutToAdd(Group*,int)), m_db);
        disconnect(SIGNAL(added()), m_db);
        disconnect(SIGNAL(aboutToMove(Group*,Group*,int)), m_db);
        disconnect(SIGNAL(moved()), m_db);
        disconnect(SIGNAL(modified()), m_db);
    }

    for (Entry* entry : asConst(m_entries)) {
        if (m_db) {
            entry->disconnect(m_db);
        }
        if (db) {
            connect(entry, SIGNAL(modified()), db, SIGNAL(modifiedImmediate()));
        }
    }

    if (db) {
        connect(this, SIGNAL(dataChanged(Group*)), db, SIGNAL(groupDataChanged(Group*)));
        connect(this, SIGNAL(aboutToRemove(Group*)), db, SIGNAL(groupAboutToRemove(Group*)));
        connect(this, SIGNAL(removed()), db, SIGNAL(groupRemoved()));
        connect(this, SIGNAL(aboutToAdd(Group*,int)), db, SIGNAL(groupAboutToAdd(Group*,int)));
        connect(this, SIGNAL(added()), db, SIGNAL(groupAdded()));
        connect(this, SIGNAL(aboutToMove(Group*,Group*,int)), db, SIGNAL(groupAboutToMove(Group*,Group*,int)));
        connect(this, SIGNAL(moved()), db, SIGNAL(groupMoved()));
        connect(this, SIGNAL(modified()), db, SIGNAL(modifiedImmediate()));
    }

    m_db = db;

    for (Group* group : asConst(m_children)) {
        group->recSetDatabase(db);
    }
}

void Group::cleanupParent()
{
    if (m_parent) {
        emit aboutToRemove(this);
        m_parent->m_children.removeAll(this);
        emit modified();
        emit removed();
    }
}

void Group::recCreateDelObjects()
{
    if (m_db) {
        for (Entry* entry : asConst(m_entries)) {
            m_db->addDeletedObject(entry->uuid());
        }

        for (Group* group : asConst(m_children)) {
            group->recCreateDelObjects();
        }
        m_db->addDeletedObject(m_uuid);
    }
}

void Group::markOlderEntry(Entry* entry)
{
    entry->attributes()->set(
        "merged",
        QString("older entry merged from database \"%1\"").arg(entry->group()->database()->metadata()->name()));
}

bool Group::resolveSearchingEnabled() const
{
    switch (m_data.searchingEnabled) {
    case Inherit:
        if (!m_parent) {
            return true;
        }
        else {
            return m_parent->resolveSearchingEnabled();
        }
    case Enable:
        return true;
    case Disable:
        return false;
    default:
        Q_ASSERT(false);
        return false;
    }
}

bool Group::resolveAutoTypeEnabled() const
{
    switch (m_data.autoTypeEnabled) {
    case Inherit:
        if (!m_parent) {
            return true;
        }
        else {
            return m_parent->resolveAutoTypeEnabled();
        }
    case Enable:
        return true;
    case Disable:
        return false;
    default:
        Q_ASSERT(false);
        return false;
    }
}

void Group::resolveEntryConflict(Entry* existingEntry, Entry* otherEntry)
{
    const QDateTime timeExisting = existingEntry->timeInfo().lastModificationTime();
    const QDateTime timeOther = otherEntry->timeInfo().lastModificationTime();

    Entry* clonedEntry;

    switch (mergeMode()) {
    case KeepBoth:
        // if one entry is newer, create a clone and add it to the group
        if (timeExisting > timeOther) {
            clonedEntry = otherEntry->clone(Entry::CloneNewUuid | Entry::CloneIncludeHistory);
            clonedEntry->setGroup(this);
            markOlderEntry(clonedEntry);
        } else if (timeExisting < timeOther) {
            clonedEntry = otherEntry->clone(Entry::CloneNewUuid | Entry::CloneIncludeHistory);
            clonedEntry->setGroup(this);
            markOlderEntry(existingEntry);
        }
        break;
    case KeepNewer:
        if (timeExisting < timeOther) {
            qDebug("Updating entry %s.", qPrintable(existingEntry->title()));
            // only if other entry is newer, replace existing one
            Group* currentGroup = existingEntry->group();
            currentGroup->removeEntry(existingEntry);
            otherEntry->clone(Entry::CloneIncludeHistory)->setGroup(currentGroup);
        }

        break;
    case KeepExisting:
        break;
    default:
        // do nothing
        break;
    }
}

void Group::resolveGroupConflict(Group* existingGroup, Group* otherGroup)
{
    const QDateTime timeExisting = existingGroup->timeInfo().lastModificationTime();
    const QDateTime timeOther = otherGroup->timeInfo().lastModificationTime();

    // only if the other group is newer, update the existing one.
    if (timeExisting < timeOther) {
        qDebug("Updating group %s.", qPrintable(existingGroup->name()));
        existingGroup->setName(otherGroup->name());
        existingGroup->setNotes(otherGroup->notes());
        if (otherGroup->iconNumber() == 0) {
          existingGroup->setIcon(otherGroup->iconUuid());
        } else {
          existingGroup->setIcon(otherGroup->iconNumber());
        }
        existingGroup->setExpiryTime(otherGroup->timeInfo().expiryTime());
    }

}

QStringList Group::locate(QString locateTerm, QString currentPath)
{
    Q_ASSERT(!locateTerm.isNull());
    QStringList response;

    for (Entry* entry : asConst(m_entries)) {
        QString entryPath = currentPath + entry->title();
        if (entryPath.toLower().contains(locateTerm.toLower())) {
            response << entryPath;
        }
    }

    for (Group* group : asConst(m_children)) {
        for (QString path : group->locate(locateTerm, currentPath + group->name() + QString("/"))) {
            response << path;
        }
    }

    return response;
}

Entry* Group::addEntryWithPath(QString entryPath)
{
    Q_ASSERT(!entryPath.isNull());
    if (this->findEntryByPath(entryPath)) {
        return nullptr;
    }

    QStringList groups = entryPath.split("/");
    QString entryTitle = groups.takeLast();
    QString groupPath = groups.join("/");
    if (groupPath.isNull()) {
        groupPath = QString("");
    }

    Q_ASSERT(!groupPath.isNull());
    Group* group = this->findGroupByPath(groupPath);
    if (!group) {
        return nullptr;
    }

    Entry* entry = new Entry();
    entry->setTitle(entryTitle);
    entry->setUuid(Uuid::random());
    entry->setGroup(group);

    return entry;

}
