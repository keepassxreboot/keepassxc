/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "Collection.h"

#include "fdosecrets/FdoSecretsPlugin.h"
#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"

#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"

#include <QEventLoop>
#include <QFileInfo>

namespace FdoSecrets
{
    Collection* Collection::Create(Service* parent, const QString& name)
    {
        QScopedPointer<Collection> res{new Collection(parent, name)};
        if (!res->dbus()->registerObject(res.data())) {
            return nullptr;
        }

        return res.take();
    }

    Collection::Collection(Service* parent, const QString& name)
        : DBusObject(parent)
        , m_backend()
        , m_name{name}
        , m_exposedGroup()
        , m_items{}
        , m_entryToItem{}
    {
    }

    Collection::~Collection() = default;

    void Collection::reloadBackend()
    {
        Q_ASSERT(m_backend);

        // delete all items
        // this has to be done because the backend is actually still there, just we don't expose them
        // NOTE: Do NOT use a for loop, because Item::removeFromDBus will remove itself from m_items.
        while (!m_items.isEmpty()) {
            m_items.first()->removeFromDBus();
        }
        cleanupConnections();
        populateContents();

        emit collectionChanged();
    }

    DBusResult Collection::ensureBackend() const
    {
        if (!m_backend) {
            return DBusResult(DBUS_ERROR_SECRET_IS_LOCKED);
        }
        return {};
    }

    /**
     * D-Bus Properties
     */

    DBusResult Collection::items(QList<Item*>& items) const
    {
        auto ret = ensureBackend();
        if (ret.err()) {
            return ret;
        }
        items = m_items;
        return {};
    }

    DBusResult Collection::label(QString& label) const
    {
        auto ret = ensureBackend();
        if (ret.err()) {
            return ret;
        }

        if (!m_backend) {
            label = name();
        } else {
            label = m_backend->metadata()->name();
        }

        return {};
    }

    DBusResult Collection::setLabel(const QString& label)
    {
        auto ret = ensureBackend();
        if (ret.err()) {
            return ret;
        }

        m_backend->metadata()->setName(label);
        return {};
    }

    DBusResult Collection::locked(bool& locked) const
    {
        locked = !m_backend;
        return {};
    }

    DBusResult Collection::created(qulonglong& created) const
    {
        auto ret = ensureBackend();
        if (ret.err()) {
            return ret;
        }
        created = static_cast<qulonglong>(m_backend->rootGroup()->timeInfo().creationTime().toMSecsSinceEpoch() / 1000);

        return {};
    }

    DBusResult Collection::modified(qulonglong& modified) const
    {
        auto ret = ensureBackend();
        if (ret.err()) {
            return ret;
        }
        // FIXME: there seems not to have a global modified time.
        // Use a more accurate time, considering all metadata, group, entry.
        modified = static_cast<qulonglong>(m_backend->rootGroup()->timeInfo().lastModificationTime().toMSecsSinceEpoch()
                                           / 1000);
        return {};
    }

    /**
     * D-Bus Methods
     */

    DBusResult Collection::remove(PromptBase*& prompt)
    {
        auto ret = ensureBackend();
        if (ret.err()) {
            return ret;
        }

        prompt = PromptBase::Create<DeleteCollectionPrompt>(service(), this);
        if (!prompt) {
            return QDBusError::InternalError;
        }
        return {};
    }

    DBusResult Collection::searchItems(const DBusClientPtr&, const StringStringMap& attributes, QList<Item*>& items)
    {
        items.clear();

        auto ret = ensureBackend();
        if (ret.err()) {
            return ret;
        }

        // shortcut logic for Uuid/Path attributes, as they can uniquely identify an item.
        if (attributes.contains(ItemAttributes::UuidKey)) {
            auto uuid = QUuid::fromRfc4122(QByteArray::fromHex(attributes.value(ItemAttributes::UuidKey).toLatin1()));
            auto entry = m_exposedGroup->findEntryByUuid(uuid);
            if (entry) {
                items += m_entryToItem.value(entry);
            }
            return {};
        }

        if (attributes.contains(ItemAttributes::PathKey)) {
            auto path = attributes.value(ItemAttributes::PathKey);
            auto entry = m_exposedGroup->findEntryByPath(path);
            if (entry) {
                items += m_entryToItem.value(entry);
            }
            return {};
        }

        QList<EntrySearcher::SearchTerm> terms;
        for (auto it = attributes.constBegin(); it != attributes.constEnd(); ++it) {
            terms << attributeToTerm(it.key(), it.value());
        }

        constexpr auto caseSensitive = false;
        constexpr auto skipProtected = true;
        constexpr auto forceSearch = true;
        const auto foundEntries =
            EntrySearcher(caseSensitive, skipProtected).search(terms, m_exposedGroup, forceSearch);
        items.reserve(foundEntries.size());
        for (const auto& entry : foundEntries) {
            const auto item = m_entryToItem.value(entry);
            // it's possible that we don't have a corresponding item for the entry
            // this can happen when the recycle bin is below the exposed group.
            if (item) {
                items << item;
            }
        }
        return {};
    }

    EntrySearcher::SearchTerm Collection::attributeToTerm(const QString& key, const QString& value)
    {
        static QMap<QString, EntrySearcher::Field> attrKeyToField{
            {EntryAttributes::TitleKey, EntrySearcher::Field::Title},
            {EntryAttributes::UserNameKey, EntrySearcher::Field::Username},
            {EntryAttributes::URLKey, EntrySearcher::Field::Url},
            {EntryAttributes::NotesKey, EntrySearcher::Field::Notes},
        };

        EntrySearcher::SearchTerm term{};
        term.field = attrKeyToField.value(key, EntrySearcher::Field::AttributeValue);
        term.word = key;
        term.exclude = false;
        term.regex =
            Tools::convertToRegex(value,
                                  Tools::RegexConvertOpts::EXACT_MATCH | Tools::RegexConvertOpts::CASE_SENSITIVE
                                      | Tools::RegexConvertOpts::ESCAPE_REGEX);

        return term;
    }

    DBusResult Collection::createItem(const QVariantMap& properties,
                                      const Secret& secret,
                                      bool replace,
                                      Item*& item,
                                      PromptBase*& prompt)
    {
        item = nullptr;

        prompt = PromptBase::Create<CreateItemPrompt>(service(), this, properties, secret, replace);
        if (!prompt) {
            return QDBusError::InternalError;
        }
        return {};
    }

    DBusResult Collection::setProperties(const QVariantMap& properties)
    {
        auto label = properties.value(DBUS_INTERFACE_SECRET_COLLECTION + ".Label").toString();

        auto ret = setLabel(label);
        if (ret.err()) {
            return ret;
        }

        return {};
    }

    void Collection::populateContents()
    {
        if (!m_backend) {
            return;
        }

        // we have an unlocked db

        auto newUuid = FdoSecrets::settings()->exposedGroup(m_backend.data());
        auto newGroup = m_backend->rootGroup()->findGroupByUuid(newUuid);
        if (!newGroup || newGroup->isRecycled()) {
            // no exposed group, delete self
            removeFromDBus();
            return;
        }

        // clean up old group connections
        cleanupConnections();

        m_exposedGroup = newGroup;

        // Attach signal to update exposed group settings if the group was removed.
        //
        // When the group object is normally deleted due to ~Database, the databaseReplaced
        // signal should be first emitted, and we will clean up connection in reloadDatabase,
        // so this handler won't be triggered.
        connect(m_exposedGroup.data(), &Group::groupAboutToRemove, this, [this](Group* toBeRemoved) {
            if (!m_backend) {
                return;
            }

            Q_ASSERT(toBeRemoved->database() == m_backend);
            if (toBeRemoved->database() != m_backend) {
                // should not happen, but anyway.
                // somehow our current database has been changed, and the old group is being deleted
                // possibly logic changes in replaceDatabase.
                return;
            }
            auto uuid = FdoSecrets::settings()->exposedGroup(m_backend.data());
            auto exposedGroup = m_backend->rootGroup()->findGroupByUuid(uuid);
            if (toBeRemoved == exposedGroup) {
                // reset the exposed group to none
                FdoSecrets::settings()->setExposedGroup(m_backend.data(), {});
            }
        });
        // Another possibility is the group being moved to recycle bin.
        connect(m_exposedGroup.data(), &Group::modified, this, [this]() {
            if (m_exposedGroup->isRecycled()) {
                // reset the exposed group to none
                FdoSecrets::settings()->setExposedGroup(m_backend.data(), {});
            }
        });

        // Monitor exposed group settings
        connect(m_backend->metadata()->customData(), &CustomData::modified, this, [this]() {
            if (!m_exposedGroup || !m_backend) {
                return;
            }
            if (m_exposedGroup->uuid() == FdoSecrets::settings()->exposedGroup(m_backend.data())) {
                // no change
                return;
            }
            reloadBackend();
        });

        // Add items for existing entry
        const auto entries = m_exposedGroup->entriesRecursive(false);
        for (const auto& entry : entries) {
            onEntryAdded(entry);
        }

        // Do not connect to Database::modified signal because we only want signals for the subset under m_exposedGroup
        connect(m_backend->metadata(), &Metadata::modified, this, &Collection::collectionChanged);
        connectGroupSignalRecursive(m_exposedGroup);
    }

    Item* Collection::onEntryAdded(Entry* entry)
    {
        if (entry->isRecycled()) {
            return nullptr;
        }

        auto item = m_entryToItem.value(entry);
        // Entry already has corresponding Item
        if (item) {
            return item;
        }

        item = Item::Create(this, entry);
        if (!item) {
            return nullptr;
        }

        m_items << item;
        m_entryToItem[entry] = item;

        // relay signals
        connect(item, &Item::itemChanged, this, [this, item]() { emit itemChanged(item); });
        connect(item, &Item::itemAboutToDelete, this, [this, item]() {
            m_items.removeAll(item);
            m_entryToItem.remove(item->backend());
            emit itemDeleted(item);
        });

        return item;
    }

    void Collection::connectGroupSignalRecursive(Group* group)
    {
        if (group->isRecycled()) {
            return;
        }

        connect(group, &Group::modified, this, &Collection::collectionChanged);
        connect(group, &Group::entryAdded, this, [this](Entry* entry) {
            if (Item* item = onEntryAdded(entry)) {
                emit itemCreated(item);
            }
        });

        const auto children = group->children();
        for (const auto& cg : children) {
            connectGroupSignalRecursive(cg);
        }
    }

    FdoSecretsPlugin* Collection::plugin() const
    {
        return service()->plugin();
    }

    bool Collection::doLock(const DBusClientPtr& client) const
    {
        // Already locked
        if (!m_backend) {
            return true;
        }

        return plugin()->doLockDatabase(client, m_name);
    }

    bool Collection::doUnlock(const DBusClientPtr& client) const
    {
        // Already unlocked
        if (m_backend) {
            return true;
        }

        return plugin()->doUnlockDatabase(client, m_name);
    }

    void Collection::removeFromDBus()
    {
        if (!m_backend && !m_exposedGroup) {
            // I'm already deleted
            return;
        }

        emit collectionAboutToDelete();

        // remove from D-Bus early
        dbus()->unregisterObject(this);

        // cleanup connection on Database
        cleanupConnections();

        // items will be removed automatically as they are children objects
        m_items.clear();

        parent()->disconnect(this);

        m_exposedGroup = nullptr;

        // reset database and delete self
        m_backend = nullptr;

        deleteLater();
    }

    void Collection::cleanupConnections()
    {
        if (m_backend && m_backend->metadata() && m_backend->metadata()->customData()) {
            m_backend->metadata()->customData()->disconnect(this);
        }

        if (m_exposedGroup) {
            for (const auto group : m_exposedGroup->groupsRecursive(true)) {
                group->disconnect(this);
            }
        }
    }

    QString Collection::dbusName() const
    {
        return QFileInfo(m_name).completeBaseName();
    }

    Group* Collection::exposedRootGroup() const
    {
        return m_exposedGroup;
    }

    bool Collection::doDeleteItem(const DBusClientPtr& client, const Item* item) const
    {
        Q_ASSERT(m_backend);

        auto entry = item->backend();
        bool permanent = entry->isRecycled() || !m_backend->metadata()->recycleBinEnabled();
        return plugin()->requestEntriesRemove(client, m_name, {entry}, permanent);
    }

    Group* Collection::findCreateGroupByPath(const QString& groupPath)
    {
        auto group = m_exposedGroup->findGroupByPath(groupPath);
        if (group) {
            return group;
        }

        // groupPath can't be empty here, because otherwise it will match m_exposedGroup and was returned above
        Q_ASSERT(!groupPath.isEmpty());

        auto groups = groupPath.split('/', Qt::SkipEmptyParts);
        auto groupName = groups.takeLast();

        // create parent group
        auto parentGroup = findCreateGroupByPath(groups.join('/'));

        // create this group
        group = new Group();
        group->setUuid(QUuid::createUuid());
        group->setName(groupName);
        group->setIcon(Group::DefaultIconNumber);
        group->setParent(parentGroup);

        return group;
    }

    Item* Collection::doNewItem(const DBusClientPtr& client, QString itemPath)
    {
        Q_ASSERT(m_backend);

        // normalize itemPath
        itemPath = (itemPath.startsWith('/') ? QString{} : QStringLiteral("/")) + itemPath;

        // split itemPath to groupPath and itemName
        auto components = itemPath.split('/');
        Q_ASSERT(components.size() >= 2);

        auto itemName = components.takeLast();
        Group* group = findCreateGroupByPath(components.join('/'));

        // create new Entry in backend
        auto* entry = new Entry();
        entry->setUuid(QUuid::createUuid());
        entry->setTitle(itemName);
        entry->setUsername(m_backend->metadata()->defaultUserName());
        group->applyGroupIconOnCreateTo(entry);

        entry->setGroup(group);

        // the item was just created so there is no point in having it not authorized
        client->setItemAuthorized(entry->uuid(), AuthDecision::Allowed);

        // when creation finishes in backend, we will already have item
        auto created = m_entryToItem.value(entry);
        return created;
    }

    void Collection::databaseLocked()
    {
        if (!m_backend) {
            return;
        }

        m_backend = nullptr;
        emit collectionChanged();
        emit collectionLockChanged(false);
    }

    void Collection::databaseUnlocked(QSharedPointer<Database> db)
    {
        Q_ASSERT(!m_backend);
        m_backend = db;
        reloadBackend();
        emit collectionLockChanged(true);
    }

} // namespace FdoSecrets
