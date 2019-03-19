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

#include "EntryModel.h"

#include <QDateTime>
#include <QFont>
#include <QFontMetrics>
#include <QMimeData>
#include <QPainter>
#include <QPalette>

#include "core/Config.h"
#include "core/DatabaseIcons.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Metadata.h"

EntryModel::EntryModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_group(nullptr)
    , m_hideUsernames(false)
    , m_hidePasswords(true)
    , HiddenContentDisplay(QString("\u25cf").repeated(6))
    , DateFormat(Qt::DefaultLocaleShortDate)
{
}

Entry* EntryModel::entryFromIndex(const QModelIndex& index) const
{
    Q_ASSERT(index.isValid() && index.row() < m_entries.size());
    return m_entries.at(index.row());
}

QModelIndex EntryModel::indexFromEntry(Entry* entry) const
{
    int row = m_entries.indexOf(entry);
    Q_ASSERT(row != -1);
    return index(row, 1);
}

void EntryModel::setGroup(Group* group)
{
    if (!group || group == m_group) {
        return;
    }

    beginResetModel();

    severConnections();

    m_group = group;
    m_allGroups.clear();
    m_entries = group->entries();
    m_orgEntries.clear();

    makeConnections(group);

    endResetModel();
}

void EntryModel::setEntries(const QList<Entry*>& entries)
{
    beginResetModel();

    severConnections();

    m_group = nullptr;
    m_allGroups.clear();
    m_entries = entries;
    m_orgEntries = entries;

    QSet<Database*> databases;

    for (Entry* entry : asConst(m_entries)) {
        databases.insert(entry->group()->database());
    }

    for (Database* db : asConst(databases)) {
        Q_ASSERT(db);
        const QList<Group*> groupList = db->rootGroup()->groupsRecursive(true);
        for (const Group* group : groupList) {
            m_allGroups.append(group);
        }

        if (db->metadata()->recycleBin()) {
            m_allGroups.removeOne(db->metadata()->recycleBin());
        }
    }

    for (const Group* group : asConst(m_allGroups)) {
        makeConnections(group);
    }

    endResetModel();
}

int EntryModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        return m_entries.size();
    }
}

int EntryModel::columnCount(const QModelIndex& parent) const
{
    // Advised by Qt documentation
    if (parent.isValid()) {
        return 0;
    }

    return 13;
}

QVariant EntryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Entry* entry = entryFromIndex(index);
    EntryAttributes* attr = entry->attributes();

    if (role == Qt::DisplayRole) {
        QString result;
        switch (index.column()) {
        case ParentGroup:
            if (entry->group()) {
                return entry->group()->name();
            }
            break;
        case Title:
            result = entry->resolveMultiplePlaceholders(entry->title());
            if (attr->isReference(EntryAttributes::TitleKey)) {
                result.prepend(tr("Ref: ", "Reference abbreviation"));
            }
            return result;
        case Username:
            if (m_hideUsernames) {
                result = EntryModel::HiddenContentDisplay;
            } else {
                result = entry->resolveMultiplePlaceholders(entry->username());
            }
            if (attr->isReference(EntryAttributes::UserNameKey)) {
                result.prepend(tr("Ref: ", "Reference abbreviation"));
            }
            return result;
        case Password:
            if (m_hidePasswords) {
                result = EntryModel::HiddenContentDisplay;
            } else {
                result = entry->resolveMultiplePlaceholders(entry->password());
            }
            if (attr->isReference(EntryAttributes::PasswordKey)) {
                result.prepend(tr("Ref: ", "Reference abbreviation"));
            }
            if (entry->password().isEmpty() && config()->get("security/passwordemptynodots").toBool()) {
                result = "";
            }
            return result;
        case Url:
            result = entry->resolveMultiplePlaceholders(entry->displayUrl());
            if (attr->isReference(EntryAttributes::URLKey)) {
                result.prepend(tr("Ref: ", "Reference abbreviation"));
            }
            return result;
        case Notes:
            // Display only first line of notes in simplified format
            result = entry->notes().section("\n", 0, 0).simplified();
            if (attr->isReference(EntryAttributes::NotesKey)) {
                result.prepend(tr("Ref: ", "Reference abbreviation"));
            }
            return result;
        case Expires:
            // Display either date of expiry or 'Never'
            result = entry->timeInfo().expires()
                         ? entry->timeInfo().expiryTime().toLocalTime().toString(EntryModel::DateFormat)
                         : tr("Never");
            return result;
        case Created:
            result = entry->timeInfo().creationTime().toLocalTime().toString(EntryModel::DateFormat);
            return result;
        case Modified:
            result = entry->timeInfo().lastModificationTime().toLocalTime().toString(EntryModel::DateFormat);
            return result;
        case Accessed:
            result = entry->timeInfo().lastAccessTime().toLocalTime().toString(EntryModel::DateFormat);
            return result;
        case Attachments: {
            // Display comma-separated list of attachments
            QList<QString> attachments = entry->attachments()->keys();
            for (const auto& attachment : attachments) {
                if (result.isEmpty()) {
                    result.append(attachment);
                    continue;
                }
                result.append(QString(", ") + attachment);
            }
            return result;
        }
        case Totp:
            result = entry->hasTotp() ? tr("Yes") : "";
            return result;
        }
    } else if (role == Qt::UserRole) { // Qt::UserRole is used as sort role, see EntryView::EntryView()
        switch (index.column()) {
        case Username:
            return entry->resolveMultiplePlaceholders(entry->username());
        case Password:
            return entry->resolveMultiplePlaceholders(entry->password());
        case Expires:
            // There seems to be no better way of expressing 'infinity'
            return entry->timeInfo().expires() ? entry->timeInfo().expiryTime() : QDateTime(QDate(9999, 1, 1));
        case Created:
            return entry->timeInfo().creationTime();
        case Modified:
            return entry->timeInfo().lastModificationTime();
        case Accessed:
            return entry->timeInfo().lastAccessTime();
        case Paperclip:
            // Display entries with attachments above those without when
            // sorting ascendingly (and vice versa when sorting descendingly)
            return entry->attachments()->isEmpty() ? 1 : 0;
        default:
            // For all other columns, simply use data provided by Qt::Display-
            // Role for sorting
            return data(index, Qt::DisplayRole);
        }
    } else if (role == Qt::DecorationRole) {
        switch (index.column()) {
        case ParentGroup:
            if (entry->group()) {
                return entry->group()->iconScaledPixmap();
            }
            break;
        case Title:
            if (entry->isExpired()) {
                return databaseIcons()->iconPixmap(DatabaseIcons::ExpiredIconIndex);
            }
            return entry->iconScaledPixmap();
        case Paperclip:
            if (!entry->attachments()->isEmpty()) {
                return m_paperClipPixmap;
            }
            break;
        }
    } else if (role == Qt::FontRole) {
        QFont font;
        if (entry->isExpired()) {
            font.setStrikeOut(true);
        }
        return font;
    } else if (role == Qt::ForegroundRole) {
        if (entry->hasReferences()) {
            QPalette p;
            return QVariant(p.color(QPalette::Active, QPalette::Mid));
        } else if (entry->foregroundColor().isValid()) {
            return QVariant(entry->foregroundColor());
        }
    } else if (role == Qt::BackgroundRole) {
        if (entry->backgroundColor().isValid()) {
            return QVariant(entry->backgroundColor());
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == Paperclip) {
            return Qt::AlignCenter;
        }
    }

    return QVariant();
}

QVariant EntryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    if (role == Qt::DisplayRole) {
        switch (section) {
        case ParentGroup:
            return tr("Group");
        case Title:
            return tr("Title");
        case Username:
            return tr("Username");
        case Password:
            return tr("Password");
        case Url:
            return tr("URL");
        case Notes:
            return tr("Notes");
        case Expires:
            return tr("Expires");
        case Created:
            return tr("Created");
        case Modified:
            return tr("Modified");
        case Accessed:
            return tr("Accessed");
        case Attachments:
            return tr("Attachments");
        case Totp:
            return tr("TOTP");
        }
    } else if (role == Qt::DecorationRole) {
        if (section == Paperclip) {
            return m_paperClipPixmap;
        }
    }

    return QVariant();
}

Qt::DropActions EntryModel::supportedDropActions() const
{
    return Qt::IgnoreAction;
}

Qt::DropActions EntryModel::supportedDragActions() const
{
    return (Qt::MoveAction | Qt::CopyAction);
}

Qt::ItemFlags EntryModel::flags(const QModelIndex& modelIndex) const
{
    if (!modelIndex.isValid()) {
        return Qt::NoItemFlags;
    } else {
        return QAbstractItemModel::flags(modelIndex) | Qt::ItemIsDragEnabled;
    }
}

QStringList EntryModel::mimeTypes() const
{
    QStringList types;
    types << QLatin1String("application/x-keepassx-entry");
    return types;
}

QMimeData* EntryModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }

    QMimeData* data = new QMimeData();
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);

    QSet<Entry*> seenEntries;

    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }

        Entry* entry = entryFromIndex(index);
        if (!seenEntries.contains(entry)) {
            // make sure we don't add entries multiple times when we get indexes
            // with the same row but different columns
            stream << entry->group()->database()->uuid() << entry->uuid();
            seenEntries.insert(entry);
        }
    }

    if (seenEntries.isEmpty()) {
        delete data;
        return nullptr;
    } else {
        data->setData(mimeTypes().at(0), encoded);
        return data;
    }
}

void EntryModel::entryAboutToAdd(Entry* entry)
{
    if (!m_group && !m_orgEntries.contains(entry)) {
        return;
    }

    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    if (!m_group) {
        m_entries.append(entry);
    }
}

void EntryModel::entryAdded(Entry* entry)
{
    if (!m_group && !m_orgEntries.contains(entry)) {
        return;
    }

    if (m_group) {
        m_entries = m_group->entries();
    }
    endInsertRows();
}

void EntryModel::entryAboutToRemove(Entry* entry)
{
    beginRemoveRows(QModelIndex(), m_entries.indexOf(entry), m_entries.indexOf(entry));
    if (!m_group) {
        m_entries.removeAll(entry);
    }
}

void EntryModel::entryRemoved()
{
    if (m_group) {
        m_entries = m_group->entries();
    }

    endRemoveRows();
}

void EntryModel::entryDataChanged(Entry* entry)
{
    int row = m_entries.indexOf(entry);
    emit dataChanged(index(row, 0), index(row, columnCount() - 1));
}

void EntryModel::severConnections()
{
    if (m_group) {
        disconnect(m_group, nullptr, this, nullptr);
    }

    for (const Group* group : asConst(m_allGroups)) {
        disconnect(group, nullptr, this, nullptr);
    }
}

void EntryModel::makeConnections(const Group* group)
{
    connect(group, SIGNAL(entryAboutToAdd(Entry*)), SLOT(entryAboutToAdd(Entry*)));
    connect(group, SIGNAL(entryAdded(Entry*)), SLOT(entryAdded(Entry*)));
    connect(group, SIGNAL(entryAboutToRemove(Entry*)), SLOT(entryAboutToRemove(Entry*)));
    connect(group, SIGNAL(entryRemoved(Entry*)), SLOT(entryRemoved()));
    connect(group, SIGNAL(entryDataChanged(Entry*)), SLOT(entryDataChanged(Entry*)));
}

/**
 * Get current state of 'Hide Usernames' setting
 */
bool EntryModel::isUsernamesHidden() const
{
    return m_hideUsernames;
}

/**
 * Set state of 'Hide Usernames' setting and signal change
 */
void EntryModel::setUsernamesHidden(bool hide)
{
    m_hideUsernames = hide;
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    emit usernamesHiddenChanged();
}

/**
 * Get current state of 'Hide Passwords' setting
 */
bool EntryModel::isPasswordsHidden() const
{
    return m_hidePasswords;
}

/**
 * Set state of 'Hide Passwords' setting and signal change
 */
void EntryModel::setPasswordsHidden(bool hide)
{
    m_hidePasswords = hide;
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    emit passwordsHiddenChanged();
}

void EntryModel::setPaperClipPixmap(const QPixmap& paperclip)
{
    m_paperClipPixmap = paperclip;
}
