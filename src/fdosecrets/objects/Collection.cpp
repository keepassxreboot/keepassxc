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

#include "fdosecrets/FdoSecretsSettings.h"
#include "fdosecrets/objects/Item.h"
#include "fdosecrets/objects/Prompt.h"
#include "fdosecrets/objects/Service.h"

#include "core/Config.h"
#include "core/Database.h"
#include "core/Tools.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"

#include <QFileInfo>

namespace FdoSecrets
{

    Collection::Collection(Service* parent, DatabaseWidget* backend)
        : DBusObject(parent)
        , m_backend(backend)
        , m_exposedGroup(nullptr)
        , m_registered(false)
    {
        // whenever the file path or the database object itself change, we do a full reload.
        connect(backend, &DatabaseWidget::databaseFilePathChanged, this, &Collection::reloadBackend);
        connect(backend, &DatabaseWidget::databaseReplaced, this, &Collection::reloadBackend);

        // also remember to clear/populate the database when lock state changes.
        connect(backend, &DatabaseWidget::databaseUnlocked, this, &Collection::onDatabaseLockChanged);
        connect(backend, &DatabaseWidget::databaseLocked, this, &Collection::onDatabaseLockChanged);

        reloadBackend();
    }

    void Collection::reloadBackend()
    {
        if (m_registered) {
            // delete all items
            // this has to be done because the backend is actually still there, just we don't expose them
            // NOTE: Do NOT use a for loop, because Item::doDelete will remove itself from m_items.
            while (!m_items.isEmpty()) {
                m_items.first()->doDelete();
            }
            cleanupConnections();

            unregisterCurrentPath();
            m_registered = false;
        }

        // make sure we have updated copy of the filepath, which is used to identify the database.
        m_backendPath = m_backend->database()->filePath();

        // the database may not have a name (derived from filePath) yet, which may happen if it's newly created.
        // defer the registration to next time a file path change happens.
        if (!name().isEmpty()) {
            registerWithPath(
                QStringLiteral(DBUS_PATH_TEMPLATE_COLLECTION).arg(p()->objectPath().path(), encodePath(name())),
                new CollectionAdaptor(this));
            m_registered = true;
        }

        // populate contents after expose on dbus, because items rely on parent's dbus object path
        if (!backendLocked()) {
            populateContents();
        } else {
            cleanupConnections();
        }
    }

    DBusReturn<void> Collection::ensureBackend() const
    {
        if (!m_backend) {
            return DBusReturn<>::Error(QStringLiteral(DBUS_ERROR_SECRET_NO_SUCH_OBJECT));
        }
        return {};
    }

    DBusReturn<void> Collection::ensureUnlocked() const
    {
        if (backendLocked()) {
            return DBusReturn<>::Error(QStringLiteral(DBUS_ERROR_SECRET_IS_LOCKED));
        }
        return {};
    }

    DBusReturn<const QList<Item*>> Collection::items() const
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }
        return m_items;
    }

    DBusReturn<QString> Collection::label() const
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }

        if (backendLocked()) {
            return name();
        }
        return m_backend->database()->metadata()->name();
    }

    DBusReturn<void> Collection::setLabel(const QString& label)
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }
        ret = ensureUnlocked();
        if (ret.isError()) {
            return ret;
        }

        m_backend->database()->metadata()->setName(label);
        return {};
    }

    DBusReturn<bool> Collection::locked() const
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }
        return backendLocked();
    }

    DBusReturn<qulonglong> Collection::created() const
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }
        ret = ensureUnlocked();
        if (ret.isError()) {
            return ret;
        }
        return static_cast<qulonglong>(m_backend->database()->rootGroup()->timeInfo().creationTime().toMSecsSinceEpoch()
                                       / 1000);
    }

    DBusReturn<qulonglong> Collection::modified() const
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }
        ret = ensureUnlocked();
        if (ret.isError()) {
            return ret;
        }
        // FIXME: there seems not to have a global modified time.
        // Use a more accurate time, considering all metadata, group, entry.
        return static_cast<qulonglong>(
            m_backend->database()->rootGroup()->timeInfo().lastModificationTime().toMSecsSinceEpoch() / 1000);
    }

    DBusReturn<PromptBase*> Collection::deleteCollection()
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }

        // Delete means close database
        auto prompt = new DeleteCollectionPrompt(service(), this);
        if (backendLocked()) {
            // this won't raise a dialog, immediate execute
            auto pret = prompt->prompt({});
            if (pret.isError()) {
                return pret;
            }
            prompt = nullptr;
        }
        // defer the close to the prompt
        return prompt;
    }

    DBusReturn<const QList<Item*>> Collection::searchItems(const StringStringMap& attributes)
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }
        ret = ensureUnlocked();
        if (ret.isError()) {
            // searchItems should work, whether `this` is locked or not.
            // however, we can't search items the same way as in gnome-keying,
            // because there's no database at all when locked.
            return QList<Item*>{};
        }

        // shortcut logic for Uuid/Path attributes, as they can uniquely identify an item.
        if (attributes.contains(ItemAttributes::UuidKey)) {
            auto uuid = QUuid::fromRfc4122(QByteArray::fromHex(attributes.value(ItemAttributes::UuidKey).toLatin1()));
            auto entry = m_exposedGroup->findEntryByUuid(uuid);
            if (entry) {
                return QList<Item*>{m_entryToItem.value(entry)};
            } else {
                return QList<Item*>{};
            }
        }

        if (attributes.contains(ItemAttributes::PathKey)) {
            auto path = attributes.value(ItemAttributes::PathKey);
            auto entry = m_exposedGroup->findEntryByPath(path);
            if (entry) {
                return QList<Item*>{m_entryToItem.value(entry)};
            } else {
                return QList<Item*>{};
            }
        }

        QList<EntrySearcher::SearchTerm> terms;
        for (auto it = attributes.constBegin(); it != attributes.constEnd(); ++it) {
            if (it.key() == EntryAttributes::PasswordKey) {
                continue;
            }
            terms << attributeToTerm(it.key(), it.value());
        }

        QList<Item*> items;
        const auto foundEntries = EntrySearcher().search(terms, m_exposedGroup);
        items.reserve(foundEntries.size());
        for (const auto& entry : foundEntries) {
            items << m_entryToItem.value(entry);
        }
        return items;
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

        const auto useWildcards = false;
        const auto exactMatch = true;
        const auto caseSensitive = true;
        term.regex = Tools::convertToRegex(value, useWildcards, exactMatch, caseSensitive);

        return term;
    }

    DBusReturn<Item*>
    Collection::createItem(const QVariantMap& properties, const SecretStruct& secret, bool replace, PromptBase*& prompt)
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }
        ret = ensureUnlocked();
        if (ret.isError()) {
            return ret;
        }

        prompt = nullptr;

        Item* item = nullptr;
        QString itemPath;
        StringStringMap attributes;

        // check existing item using attributes
        auto iterAttr = properties.find(QStringLiteral(DBUS_INTERFACE_SECRET_ITEM ".Attributes"));
        if (iterAttr != properties.end()) {
            attributes = qdbus_cast<StringStringMap>(iterAttr.value().value<QDBusArgument>());

            itemPath = attributes.value(ItemAttributes::PathKey);

            auto existings = searchItems(attributes);
            if (existings.isError()) {
                return existings;
            }
            if (!existings.value().isEmpty() && replace) {
                item = existings.value().front();
            }
        }

        if (!item) {
            // normalize itemPath
            itemPath = itemPath.startsWith('/') ? QString{} : QStringLiteral("/") + itemPath;

            // split itemPath to groupPath and itemName
            auto components = itemPath.split('/');
            Q_ASSERT(components.size() >= 2);

            auto itemName = components.takeLast();
            Group* group = findCreateGroupByPath(components.join('/'));

            // create new Entry in backend
            auto* entry = new Entry();
            entry->setUuid(QUuid::createUuid());
            entry->setTitle(itemName);
            entry->setUsername(m_backend->database()->metadata()->defaultUserName());
            group->applyGroupIconOnCreateTo(entry);

            entry->setGroup(group);

            // when creation finishes in backend, we will already have item
            item = m_entryToItem.value(entry, nullptr);
            Q_ASSERT(item);
        }

        ret = item->setProperties(properties);
        if (ret.isError()) {
            return ret;
        }
        ret = item->setSecret(secret);
        if (ret.isError()) {
            return ret;
        }

        return item;
    }

    DBusReturn<void> Collection::setProperties(const QVariantMap& properties)
    {
        auto label = properties.value(QStringLiteral(DBUS_INTERFACE_SECRET_COLLECTION ".Label")).toString();

        auto ret = setLabel(label);
        if (ret.isError()) {
            return ret;
        }

        return {};
    }

    const QSet<QString> Collection::aliases() const
    {
        return m_aliases;
    }

    DBusReturn<void> Collection::addAlias(QString alias)
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }

        alias = encodePath(alias);

        if (m_aliases.contains(alias)) {
            return {};
        }

        emit aliasAboutToAdd(alias);

        bool ok = QDBusConnection::sessionBus().registerObject(
            QStringLiteral(DBUS_PATH_TEMPLATE_ALIAS).arg(p()->objectPath().path(), alias), this);
        if (ok) {
            m_aliases.insert(alias);
            emit aliasAdded(alias);
        }

        return {};
    }

    DBusReturn<void> Collection::removeAlias(QString alias)
    {
        auto ret = ensureBackend();
        if (ret.isError()) {
            return ret;
        }

        alias = encodePath(alias);

        if (!m_aliases.contains(alias)) {
            return {};
        }

        QDBusConnection::sessionBus().unregisterObject(
            QStringLiteral(DBUS_PATH_TEMPLATE_ALIAS).arg(p()->objectPath().path(), alias));

        m_aliases.remove(alias);
        emit aliasRemoved(alias);

        return {};
    }

    QString Collection::name() const
    {
        return QFileInfo(m_backendPath).baseName();
    }

    DatabaseWidget* Collection::backend() const
    {
        return m_backend;
    }

    void Collection::onDatabaseLockChanged()
    {
        auto locked = backendLocked();
        if (!locked) {
            populateContents();
        } else {
            cleanupConnections();
        }
        emit collectionLockChanged(locked);
        emit collectionChanged();
    }

    void Collection::populateContents()
    {
        if (!m_registered) {
            return;
        }

        // we have an unlocked db

        auto newUuid = FdoSecrets::settings()->exposedGroup(m_backend->database());
        auto newGroup = m_backend->database()->rootGroup()->findGroupByUuid(newUuid);
        if (!newGroup) {
            // no exposed group, delete self
            doDelete();
            return;
        }

        // clean up old group connections
        cleanupConnections();

        m_exposedGroup = newGroup;

        // Attach signal to update exposed group settings if the group was removed.
        // The lifetime of the connection is bound to the database object, because
        // in Database::~Database, groups are also deleted, but we don't want to
        // trigger this.
        // This rely on the fact that QObject disconnects signals BEFORE deleting
        // children.
        QPointer<Database> db = m_backend->database().data();
        connect(m_exposedGroup.data(), &Group::groupAboutToRemove, db, [db](Group* toBeRemoved) {
            if (!db) {
                return;
            }
            auto uuid = FdoSecrets::settings()->exposedGroup(db);
            auto exposedGroup = db->rootGroup()->findGroupByUuid(uuid);
            if (toBeRemoved == exposedGroup) {
                // reset the exposed group to none
                FdoSecrets::settings()->setExposedGroup(db, {});
            }
        });

        // Monitor exposed group settings
        connect(m_backend->database()->metadata()->customData(), &CustomData::customDataModified, this, [this]() {
            if (!m_exposedGroup || !m_backend) {
                return;
            }
            if (m_exposedGroup->uuid() == FdoSecrets::settings()->exposedGroup(m_backend->database())) {
                // no change
                return;
            }
            onDatabaseExposedGroupChanged();
        });

        // Add items for existing entry
        const auto entries = m_exposedGroup->entriesRecursive(false);
        for (const auto& entry : entries) {
            onEntryAdded(entry, false);
        }

        connectGroupSignalRecursive(m_exposedGroup);
    }

    void Collection::onDatabaseExposedGroupChanged()
    {
        // delete all items
        // this has to be done because the backend is actually still there
        // just we don't expose them
        for (const auto& item : asConst(m_items)) {
            item->doDelete();
        }

        // repopulate
        Q_ASSERT(!backendLocked());
        populateContents();
    }

    void Collection::onEntryAdded(Entry* entry, bool emitSignal)
    {
        if (inRecycleBin(entry)) {
            return;
        }

        auto item = new Item(this, entry);
        m_items << item;
        m_entryToItem[entry] = item;

        // forward delete signals
        connect(entry->group(), &Group::entryAboutToRemove, item, [item](Entry* toBeRemoved) {
            if (item->backend() == toBeRemoved) {
                item->doDelete();
            }
        });

        // relay signals
        connect(item, &Item::itemChanged, this, [this, item]() { emit itemChanged(item); });
        connect(item, &Item::itemAboutToDelete, this, [this, item]() {
            m_items.removeAll(item);
            m_entryToItem.remove(item->backend());
            emit itemDeleted(item);
        });

        if (emitSignal) {
            emit itemCreated(item);
        }
    }

    void Collection::connectGroupSignalRecursive(Group* group)
    {
        if (inRecycleBin(group)) {
            return;
        }

        connect(group, &Group::groupModified, this, &Collection::collectionChanged);
        connect(group, &Group::entryAdded, this, [this](Entry* entry) { onEntryAdded(entry, true); });

        const auto children = group->children();
        for (const auto& cg : children) {
            connectGroupSignalRecursive(cg);
        }
    }

    Service* Collection::service() const
    {
        return qobject_cast<Service*>(parent());
    }

    void Collection::doLock()
    {
        Q_ASSERT(m_backend);

        m_backend->lock();
    }

    void Collection::doUnlock()
    {
        Q_ASSERT(m_backend);

        service()->doUnlockDatabaseInDialog(m_backend);
    }

    void Collection::doDelete()
    {
        emit collectionAboutToDelete();

        unregisterCurrentPath();

        // remove alias manually to trigger signal
        for (const auto& a : aliases()) {
            removeAlias(a).okOrDie();
        }

        cleanupConnections();

        m_exposedGroup = nullptr;

        // reset backend and delete self
        m_backend = nullptr;
        deleteLater();
    }

    void Collection::cleanupConnections()
    {
        if (m_exposedGroup) {
            m_exposedGroup->disconnect(this);
        }
        m_items.clear();
    }

    QString Collection::backendFilePath() const
    {
        return m_backendPath;
    }

    Group* Collection::exposedRootGroup() const
    {
        return m_exposedGroup;
    }

    bool Collection::backendLocked() const
    {
        return !m_backend || !m_backend->database()->isInitialized() || m_backend->isLocked();
    }

    void Collection::doDeleteEntries(QList<Entry*> entries)
    {
        m_backend->deleteEntries(std::move(entries));
    }

    Group* Collection::findCreateGroupByPath(const QString& groupPath)
    {
        auto group = m_exposedGroup->findGroupByPath(groupPath);
        if (group) {
            return group;
        }

        // groupPath can't be empty here, because otherwise it will match m_exposedGroup and was returned above
        Q_ASSERT(!groupPath.isEmpty());

        auto groups = groupPath.split('/', QString::SkipEmptyParts);
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

    bool Collection::inRecycleBin(Group* group) const
    {
        Q_ASSERT(m_backend);

        if (!m_backend->database()->metadata()->recycleBin()) {
            return false;
        }

        while (group) {
            if (group->uuid() == m_backend->database()->metadata()->recycleBin()->uuid()) {
                return true;
            }
            group = group->parentGroup();
        }
        return false;
    }

    bool Collection::inRecycleBin(Entry* entry) const
    {
        Q_ASSERT(entry);
        return inRecycleBin(entry->group());
    }

} // namespace FdoSecrets
