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

#include <QFont>
#include <QMimeData>
#include <QPalette>

#include "core/DatabaseIcons.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Metadata.h"

EntryModel::EntryModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_group(nullptr)
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
    emit switchedToGroupMode();
}

void EntryModel::setEntryList(const QList<Entry*>& entries)
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
    emit switchedToEntryListMode();
}

int EntryModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    else {
        return m_entries.size();
    }
}

int EntryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    /**
     * @author Fonic <https://github.com/fonic>
     * Update column count to account for additional columns -> column count = number of entries in enum 'ModelColumn' (EntryModel.h)
     */
    return 11;
}

QVariant EntryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Entry* entry = entryFromIndex(index);
    EntryAttributes* attr = entry->attributes();

    /**
     * @author Fonic <https://github.com/fonic>
     * Add display data providers for additional columns 'Password', 'Notes', 'Expires', 'Created', 'Modified', 'Accessed', 'Attachments'
     *
     * TODO: check what entry->resolveMultiplePlaceholders() does and if it's necessary/useful for the additional columns
     *       -> allows using placeholders like '{username}' that are resolved automatically
     *       -> NOT useful for timestamps and attachments
     *       -> PROBABLY useful for notes, but it could also be desirable to display notes exactly as typed...
     *
     * TODO: check what attr->isReference() does and if it's necessary/useful for the additional columns
     */
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
                result.prepend(tr("Ref: ","Reference abbreviation"));
            }
            return result;
        case Username:
            result = entry->resolveMultiplePlaceholders(entry->username());
            if (attr->isReference(EntryAttributes::UserNameKey)) {
                result.prepend(tr("Ref: ","Reference abbreviation"));
            }
            return result;
        case Password:

            // Display password in cleartext
            //result = entry->resolveMultiplePlaceholders(entry->password());

            // Display password hidden/obfuscated
            // TODO: check source code of QLineEdit to find out how they do it
            //       -> https://github.com/openwebos/qt/blob/master/src/gui/widgets/qlineedit.cpp#L2210
            //       -> QStyle::SH_LineEdit_PasswordCharacter (requires '#include <QStyle>')
            //       -> find out how to derive a QChar from QStyle::SH_LineEdit_PasswordCharacter
            //       -> https://code.woboq.org/qt5/qtbase/src/widgets/styles/qcommonstyle.cpp.html#5029
            //          -> Qt internals, does not seem to be reproducible
            //result = QString("*").repeated(6);
            //result = QString("******");
            result = QString(QChar(0x2022)).repeated(6);

            if (attr->isReference(EntryAttributes::PasswordKey)) {
                result.prepend(tr("Ref: ","Reference abbreviation"));
            }
            return result;
        case Url:
            result = entry->resolveMultiplePlaceholders(entry->url());
            if (attr->isReference(EntryAttributes::URLKey)) {
                result.prepend(tr("Ref: ","Reference abbreviation"));
            }
            return result;
        case Notes:
            // Display only first line of notes in simplified format
            //result = entry->resolveMultiplePlaceholders(entry->notes().section("\n", 0, 0).simplified());
            result = entry->notes().section("\n", 0, 0).simplified();
            if (attr->isReference(EntryAttributes::NotesKey)) {
                result.prepend(tr("Ref: ","Reference abbreviation"));
            }
            return result;
        case Expires:
            // Display either date of expiry or 'Never'
            result = entry->timeInfo().expires() ? entry->timeInfo().expiryTime().toLocalTime().toString(Qt::DefaultLocaleShortDate) : tr("Never");
            return result;
        case Created:
            result = entry->timeInfo().creationTime().toLocalTime().toString(Qt::DefaultLocaleShortDate);
            return result;
        case Modified:
            result = entry->timeInfo().lastModificationTime().toLocalTime().toString(Qt::DefaultLocaleShortDate);
            return result;
        case Accessed:
            result = entry->timeInfo().lastAccessTime().toLocalTime().toString(Qt::DefaultLocaleShortDate);
            return result;
        case Attachments:
            // Display comma-separated list of attachments
            // TODO: entry->attachments()->keys().join() works locally (http://doc.qt.io/qt-5/qlist.html#more-members),
            //       yet it fails on GitHub/Travis CI, most likely due to an older Qt version; using for loop for now
            //result = entry->attachments()->keys().join(", ");
            QList<QString> attachments = entry->attachments()->keys();
            for (int i=0; i < attachments.size(); i++) {
                if (!result.isEmpty())
                    result.append(", ");
                result.append(attachments.at(i));
            }
            return result;
        }
    }
    /**
     * @author Fonic <https://github.com/fonic>
     * Add user role to correctly sort dates -> 'm_sortModel->setSortRole(Qt::UserRole);', EntrieView.cpp
     * TODO: is there any better way to define 'Never' (=infinity/end of time)?
     */
    else if (role == Qt::UserRole) {
        switch (index.column()) {
        case Expires:
            return entry->timeInfo().expires() ? entry->timeInfo().expiryTime() : QDateTime(QDate(9999, 1, 1));
        case Created:
            return entry->timeInfo().creationTime();
        case Modified:
            return entry->timeInfo().lastModificationTime();
        case Accessed:
            return entry->timeInfo().lastAccessTime();
        default:
            // For all other columns, use data provided by Qt::DisplayRole
            return data(index, Qt::DisplayRole);
        }
    }
    else if (role == Qt::DecorationRole) {
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
            else {
                return entry->iconScaledPixmap();
            }
        }
    }
    else if (role == Qt::FontRole) {
        QFont font;
        if (entry->isExpired()) {
            font.setStrikeOut(true);
        }
        return font;
    }
    else if (role == Qt::TextColorRole) {
        if (entry->hasReferences()) {
            QPalette p;
            return QVariant(p.color(QPalette::Active, QPalette::Mid));
        }
    }

    return QVariant();
}
QVariant EntryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    /**
     * @author Fonic <https://github.com/fonic>
     * Add captions for additional columns 'Password', 'Notes', 'Expires', 'Created', 'Modified', 'Accessed', 'Attachments'
     */
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
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
        }
    }

    return QVariant();
}

Qt::DropActions EntryModel::supportedDropActions() const
{
    return 0;
}

Qt::DropActions EntryModel::supportedDragActions() const
{
    return (Qt::MoveAction | Qt::CopyAction);
}

Qt::ItemFlags EntryModel::flags(const QModelIndex& modelIndex) const
{
    if (!modelIndex.isValid()) {
        return Qt::NoItemFlags;
    }
    else {
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
    }
    else {
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
    emit dataChanged(index(row, 0), index(row, columnCount()-1));
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
