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

#include "core/Clock.h"
#include "core/Config.h"
#include "core/DatabaseIcons.h"
#include "core/Global.h"
#include "core/Metadata.h"
#include "core/Tools.h"

#include <QtConcurrent>

const int Group::DefaultIconNumber = 48;
const int Group::RecycleBinIconNumber = 43;
const QString Group::RootAutoTypeSequence = "{USERNAME}{TAB}{PASSWORD}{ENTER}";

Group::CloneFlags Group::DefaultCloneFlags =
    static_cast<Group::CloneFlags>(Group::CloneNewUuid | Group::CloneResetTimeInfo | Group::CloneIncludeEntries);
Entry::CloneFlags Group::DefaultEntryCloneFlags =
    static_cast<Entry::CloneFlags>(Entry::CloneNewUuid | Entry::CloneResetTimeInfo);

Group::Group()
    : m_customData(new CustomData(this))
    , m_updateTimeinfo(true)
{
    m_data.iconNumber = DefaultIconNumber;
    m_data.isExpanded = true;
    m_data.autoTypeEnabled = Inherit;
    m_data.searchingEnabled = Inherit;
    m_data.mergeMode = Default;

    connect(m_customData, SIGNAL(customDataModified()), this, SIGNAL(groupModified()));
    connect(this, SIGNAL(groupModified()), SLOT(updateTimeinfo()));
}

Group::~Group()
{
    setUpdateTimeinfo(false);
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
        delGroup.deletionTime = Clock::currentDateTimeUtc();
        delGroup.uuid = m_uuid;
        m_db->addDeletedObject(delGroup);
    }

    cleanupParent();
}

Group* Group::createRecycleBin()
{
    Group* recycleBin = new Group();
    recycleBin->setUuid(QUuid::createUuid());
    recycleBin->setName(tr("Recycle Bin"));
    recycleBin->setIcon(RecycleBinIconNumber);
    recycleBin->setSearchingEnabled(Group::Disable);
    recycleBin->setAutoTypeEnabled(Group::Disable);
    return recycleBin;
}

template <class P, class V> inline bool Group::set(P& property, const V& value)
{
    if (property != value) {
        property = value;
        emit groupModified();
        return true;
    } else {
        return false;
    }
}

bool Group::canUpdateTimeinfo() const
{
    return m_updateTimeinfo;
}

void Group::updateTimeinfo()
{
    if (m_updateTimeinfo) {
        m_data.timeInfo.setLastModificationTime(Clock::currentDateTimeUtc());
        m_data.timeInfo.setLastAccessTime(Clock::currentDateTimeUtc());
    }
}

void Group::setUpdateTimeinfo(bool value)
{
    m_updateTimeinfo = value;
}

const QUuid& Group::uuid() const
{
    return m_uuid;
}

const QString Group::uuidToHex() const
{
    return Tools::uuidToHex(m_uuid);
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
    } else {
        Q_ASSERT(m_db);

        if (m_db) {
            return m_db->metadata()->customIcon(m_data.customIcon);
        } else {
            return QImage();
        }
    }
}

QPixmap Group::iconPixmap() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    } else {
        Q_ASSERT(m_db);

        if (m_db) {
            return m_db->metadata()->customIconPixmap(m_data.customIcon);
        } else {
            return QPixmap();
        }
    }
}

QPixmap Group::iconScaledPixmap() const
{
    if (m_data.customIcon.isNull()) {
        // built-in icons are 16x16 so don't need to be scaled
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    } else {
        Q_ASSERT(m_db);

        if (m_db) {
            return m_db->metadata()->customIconScaledPixmap(m_data.customIcon);
        } else {
            return QPixmap();
        }
    }
}

int Group::iconNumber() const
{
    return m_data.iconNumber;
}

const QUuid& Group::iconUuid() const
{
    return m_data.customIcon;
}

const TimeInfo& Group::timeInfo() const
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
    if (m_data.mergeMode == Group::MergeMode::Default) {
        if (m_parent) {
            return m_parent->mergeMode();
        }
        return Group::MergeMode::KeepNewer; // fallback
    }
    return m_data.mergeMode;
}

Entry* Group::lastTopVisibleEntry() const
{
    return m_lastTopVisibleEntry;
}

bool Group::isExpired() const
{
    return m_data.timeInfo.expires() && m_data.timeInfo.expiryTime() < Clock::currentDateTimeUtc();
}

CustomData* Group::customData()
{
    return m_customData;
}

const CustomData* Group::customData() const
{
    return m_customData;
}

bool Group::equals(const Group* other, CompareItemOptions options) const
{
    if (!other) {
        return false;
    }
    if (m_uuid != other->m_uuid) {
        return false;
    }
    if (!m_data.equals(other->m_data, options)) {
        return false;
    }
    if (m_customData != other->m_customData) {
        return false;
    }
    if (m_children.count() != other->m_children.count()) {
        return false;
    }
    if (m_entries.count() != other->m_entries.count()) {
        return false;
    }
    for (int i = 0; i < m_children.count(); ++i) {
        if (m_children[i]->uuid() != other->m_children[i]->uuid()) {
            return false;
        }
    }
    for (int i = 0; i < m_entries.count(); ++i) {
        if (m_entries[i]->uuid() != other->m_entries[i]->uuid()) {
            return false;
        }
    }
    return true;
}

void Group::setUuid(const QUuid& uuid)
{
    set(m_uuid, uuid);
}

void Group::setName(const QString& name)
{
    if (set(m_data.name, name)) {
        emit groupDataChanged(this);
    }
}

void Group::setNotes(const QString& notes)
{
    set(m_data.notes, notes);
}

void Group::setIcon(int iconNumber)
{
    if (iconNumber >= 0 && (m_data.iconNumber != iconNumber || !m_data.customIcon.isNull())) {
        m_data.iconNumber = iconNumber;
        m_data.customIcon = QUuid();
        emit groupModified();
        emit groupDataChanged(this);
    }
}

void Group::setIcon(const QUuid& uuid)
{
    if (!uuid.isNull() && m_data.customIcon != uuid) {
        m_data.customIcon = uuid;
        m_data.iconNumber = 0;
        emit groupModified();
        emit groupDataChanged(this);
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
        emit groupModified();
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
        emit groupModified();
    }
}

void Group::setExpiryTime(const QDateTime& dateTime)
{
    if (m_data.timeInfo.expiryTime() != dateTime) {
        m_data.timeInfo.setExpiryTime(dateTime);
        emit groupModified();
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
            if (!iconUuid().isNull() && parent->m_db && m_db->metadata()->containsCustomIcon(iconUuid())
                && !parent->m_db->metadata()->containsCustomIcon(iconUuid())) {
                parent->m_db->metadata()->addCustomIcon(iconUuid(), icon());
            }
        }
        if (m_db != parent->m_db) {
            connectDatabaseSignalsRecursive(parent->m_db);
        }
        QObject::setParent(parent);
        emit groupAboutToAdd(this, index);
        Q_ASSERT(index <= parent->m_children.size());
        parent->m_children.insert(index, this);
    } else {
        emit aboutToMove(this, parent, index);
        m_parent->m_children.removeAll(this);
        m_parent = parent;
        QObject::setParent(parent);
        Q_ASSERT(index <= parent->m_children.size());
        parent->m_children.insert(index, this);
    }

    if (m_updateTimeinfo) {
        m_data.timeInfo.setLocationChanged(Clock::currentDateTimeUtc());
    }

    emit groupModified();

    if (!moveWithinDatabase) {
        emit groupAdded();
    } else {
        emit groupMoved();
    }
}

void Group::setParent(Database* db)
{
    Q_ASSERT(db);
    Q_ASSERT(db->rootGroup() == this);

    cleanupParent();

    m_parent = nullptr;
    connectDatabaseSignalsRecursive(db);

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

QList<Entry*> Group::referencesRecursive(const Entry* entry) const
{
    auto entries = entriesRecursive();
    return QtConcurrent::blockingFiltered(entries,
                                          [entry](const Entry* e) { return e->hasReferencesTo(entry->uuid()); });
}

Entry* Group::findEntryByUuid(const QUuid& uuid) const
{
    if (uuid.isNull()) {
        return nullptr;
    }

    for (Entry* entry : entriesRecursive(false)) {
        if (entry->uuid() == uuid) {
            return entry;
        }
    }

    return nullptr;
}

Entry* Group::findEntryByPath(const QString& entryPath)
{
    if (entryPath.isEmpty()) {
        return nullptr;
    }

    // Add a beginning slash if the search string contains a slash
    // We don't add a slash by default to allow searching by entry title
    QString normalizedEntryPath = entryPath;
    if (!normalizedEntryPath.startsWith("/") && normalizedEntryPath.contains("/")) {
        normalizedEntryPath = "/" + normalizedEntryPath;
    }
    return findEntryByPathRecursive(normalizedEntryPath, "/");
}

Entry* Group::findEntryBySearchTerm(const QString& term, EntryReferenceType referenceType)
{
    Q_ASSERT_X(referenceType != EntryReferenceType::Unknown,
               "Database::findEntryRecursive",
               "Can't search entry with \"referenceType\" parameter equal to \"Unknown\"");

    const QList<Group*> groups = groupsRecursive(true);

    for (const Group* group : groups) {
        bool found = false;
        const QList<Entry*>& entryList = group->entries();
        for (Entry* entry : entryList) {
            switch (referenceType) {
            case EntryReferenceType::Unknown:
                return nullptr;
            case EntryReferenceType::Title:
                found = entry->title() == term;
                break;
            case EntryReferenceType::UserName:
                found = entry->username() == term;
                break;
            case EntryReferenceType::Password:
                found = entry->password() == term;
                break;
            case EntryReferenceType::Url:
                found = entry->url() == term;
                break;
            case EntryReferenceType::Notes:
                found = entry->notes() == term;
                break;
            case EntryReferenceType::QUuid:
                found = entry->uuid() == QUuid::fromRfc4122(QByteArray::fromHex(term.toLatin1()));
                break;
            case EntryReferenceType::CustomAttributes:
                found = entry->attributes()->containsValue(term);
                break;
            }

            if (found) {
                return entry;
            }
        }
    }

    return nullptr;
}

Entry* Group::findEntryByPathRecursive(const QString& entryPath, const QString& basePath)
{
    // Return the first entry that matches the full path OR if there is no leading
    // slash, return the first entry title that matches
    for (Entry* entry : entries()) {
        // clang-format off
        if (entryPath == (basePath + entry->title())
            || (!entryPath.startsWith("/") && entry->title() == entryPath)) {
            return entry;
        }
        // clang-format on
    }

    for (Group* group : children()) {
        Entry* entry = group->findEntryByPathRecursive(entryPath, basePath + group->name() + "/");
        if (entry != nullptr) {
            return entry;
        }
    }

    return nullptr;
}

Group* Group::findGroupByPath(const QString& groupPath)
{
    // normalize the groupPath by adding missing front and rear slashes. once.
    QString normalizedGroupPath;

    if (groupPath.isEmpty()) {
        normalizedGroupPath = QString("/"); // root group
    } else {
        // clang-format off
        normalizedGroupPath = (groupPath.startsWith("/") ? "" : "/")
            + groupPath
            + (groupPath.endsWith("/") ? "" : "/");
        // clang-format on
    }
    return findGroupByPathRecursive(normalizedGroupPath, "/");
}

Group* Group::findGroupByPathRecursive(const QString& groupPath, const QString& basePath)
{
    // paths must be normalized
    Q_ASSERT(groupPath.startsWith("/") && groupPath.endsWith("/"));
    Q_ASSERT(basePath.startsWith("/") && basePath.endsWith("/"));

    if (groupPath == basePath) {
        return this;
    }

    for (Group* innerGroup : children()) {
        QString innerBasePath = basePath + innerGroup->name() + "/";
        Group* group = innerGroup->findGroupByPathRecursive(groupPath, innerBasePath);
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
        response += indentation + tr("[empty]", "group has no children") + "\n";
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

    for (const Group* group : asConst(m_children)) {
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

QSet<QUuid> Group::customIconsRecursive() const
{
    QSet<QUuid> result;

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

Group* Group::findGroupByUuid(const QUuid& uuid)
{
    if (uuid.isNull()) {
        return nullptr;
    }

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

/**
 * Creates a duplicate of this group.
 * Note that you need to copy the custom icons manually when inserting the
 * new group into another database.
 */
Group* Group::clone(Entry::CloneFlags entryFlags, Group::CloneFlags groupFlags) const
{
    Group* clonedGroup = new Group();

    clonedGroup->setUpdateTimeinfo(false);

    if (groupFlags & Group::CloneNewUuid) {
        clonedGroup->setUuid(QUuid::createUuid());
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

        QDateTime now = Clock::currentDateTimeUtc();
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
    connect(entry, SIGNAL(entryDataChanged(Entry*)), SIGNAL(entryDataChanged(Entry*)));
    if (m_db) {
        connect(entry, SIGNAL(entryModified()), m_db, SLOT(markAsModified()));
    }

    emit groupModified();
    emit entryAdded(entry);
}

void Group::removeEntry(Entry* entry)
{
    Q_ASSERT_X(m_entries.contains(entry),
               Q_FUNC_INFO,
               QString("Group %1 does not contain %2").arg(this->name(), entry->title()).toLatin1());

    emit entryAboutToRemove(entry);

    entry->disconnect(this);
    if (m_db) {
        entry->disconnect(m_db);
    }
    m_entries.removeAll(entry);
    emit groupModified();
    emit entryRemoved(entry);
}

void Group::connectDatabaseSignalsRecursive(Database* db)
{
    if (m_db) {
        disconnect(SIGNAL(groupDataChanged(Group*)), m_db);
        disconnect(SIGNAL(groupAboutToRemove(Group*)), m_db);
        disconnect(SIGNAL(groupRemoved()), m_db);
        disconnect(SIGNAL(groupAboutToAdd(Group*, int)), m_db);
        disconnect(SIGNAL(groupAdded()), m_db);
        disconnect(SIGNAL(aboutToMove(Group*, Group*, int)), m_db);
        disconnect(SIGNAL(groupMoved()), m_db);
        disconnect(SIGNAL(groupModified()), m_db);
    }

    for (Entry* entry : asConst(m_entries)) {
        if (m_db) {
            entry->disconnect(m_db);
        }
        if (db) {
            connect(entry, SIGNAL(entryModified()), db, SLOT(markAsModified()));
        }
    }

    if (db) {
        // clang-format off
        connect(this, SIGNAL(groupDataChanged(Group*)), db, SIGNAL(groupDataChanged(Group*)));
        connect(this, SIGNAL(groupAboutToRemove(Group*)), db, SIGNAL(groupAboutToRemove(Group*)));
        connect(this, SIGNAL(groupRemoved()), db, SIGNAL(groupRemoved()));
        connect(this, SIGNAL(groupAboutToAdd(Group*, int)), db, SIGNAL(groupAboutToAdd(Group*,int)));
        connect(this, SIGNAL(groupAdded()), db, SIGNAL(groupAdded()));
        connect(this, SIGNAL(aboutToMove(Group*,Group*,int)), db, SIGNAL(groupAboutToMove(Group*,Group*,int)));
        connect(this, SIGNAL(groupMoved()), db, SIGNAL(groupMoved()));
        connect(this, SIGNAL(groupModified()), db, SLOT(markAsModified()));
        // clang-format on
    }

    m_db = db;

    for (Group* group : asConst(m_children)) {
        group->connectDatabaseSignalsRecursive(db);
    }
}

void Group::cleanupParent()
{
    if (m_parent) {
        emit groupAboutToRemove(this);
        m_parent->m_children.removeAll(this);
        emit groupModified();
        emit groupRemoved();
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

bool Group::resolveSearchingEnabled() const
{
    switch (m_data.searchingEnabled) {
    case Inherit:
        if (!m_parent) {
            return true;
        } else {
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
        } else {
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

QStringList Group::locate(const QString& locateTerm, const QString& currentPath) const
{
    // TODO: Replace with EntrySearcher
    QStringList response;
    if (locateTerm.isEmpty()) {
        return response;
    }

    for (const Entry* entry : asConst(m_entries)) {
        QString entryPath = currentPath + entry->title();
        if (entryPath.contains(locateTerm, Qt::CaseInsensitive)) {
            response << entryPath;
        }
    }

    for (const Group* group : asConst(m_children)) {
        for (const QString& path : group->locate(locateTerm, currentPath + group->name() + QString("/"))) {
            response << path;
        }
    }

    return response;
}

Entry* Group::addEntryWithPath(const QString& entryPath)
{
    if (entryPath.isEmpty() || findEntryByPath(entryPath)) {
        return nullptr;
    }

    QStringList groups = entryPath.split("/");
    QString entryTitle = groups.takeLast();
    QString groupPath = groups.join("/");

    Group* group = findGroupByPath(groupPath);
    if (!group) {
        return nullptr;
    }

    auto* entry = new Entry();
    entry->setTitle(entryTitle);
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(group);

    return entry;
}

bool Group::GroupData::operator==(const Group::GroupData& other) const
{
    return equals(other, CompareItemDefault);
}

bool Group::GroupData::operator!=(const Group::GroupData& other) const
{
    return !(*this == other);
}

bool Group::GroupData::equals(const Group::GroupData& other, CompareItemOptions options) const
{
    if (::compare(name, other.name, options) != 0) {
        return false;
    }
    if (::compare(notes, other.notes, options) != 0) {
        return false;
    }
    if (::compare(iconNumber, other.iconNumber) != 0) {
        return false;
    }
    if (::compare(customIcon, other.customIcon) != 0) {
        return false;
    }
    if (timeInfo.equals(other.timeInfo, options) != 0) {
        return false;
    }
    // TODO HNH: Some properties are configurable - should they be ignored?
    if (::compare(isExpanded, other.isExpanded, options) != 0) {
        return false;
    }
    if (::compare(defaultAutoTypeSequence, other.defaultAutoTypeSequence, options) != 0) {
        return false;
    }
    if (::compare(autoTypeEnabled, other.autoTypeEnabled, options) != 0) {
        return false;
    }
    if (::compare(searchingEnabled, other.searchingEnabled, options) != 0) {
        return false;
    }
    if (::compare(mergeMode, other.mergeMode, options) != 0) {
        return false;
    }
    return true;
}
