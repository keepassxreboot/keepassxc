/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "EntryHistoryModel.h"

#include "core/Clock.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Tools.h"

#include <QFont>

EntryHistoryModel::EntryHistoryModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

Entry* EntryHistoryModel::entryFromIndex(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() >= m_historyEntries.size()) {
        return nullptr;
    }
    auto entry = m_historyEntries.at(index.row());
    return entry == m_parentEntry ? nullptr : entry;
}

int EntryHistoryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 4;
}

int EntryHistoryModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return m_historyEntries.count();
    } else {
        return 0;
    }
}

QVariant EntryHistoryModel::data(const QModelIndex& index, int role) const
{
    if (index.row() >= m_historyEntries.size()) {
        return {};
    }
    const auto entry = m_historyEntries[index.row()];

    if (role == Qt::DisplayRole || role == Qt::UserRole) {
        QDateTime lastModified = entry->timeInfo().lastModificationTime().toLocalTime();
        QDateTime now = Clock::currentDateTime();

        switch (index.column()) {
        case 0:
            if (role == Qt::DisplayRole) {
                return lastModified.toString(Qt::SystemLocaleShortDate);
            } else {
                return lastModified;
            }
        case 1: {
            const auto seconds = lastModified.secsTo(now);
            if (role == Qt::DisplayRole) {
                if (entry == m_parentEntry) {
                    return tr("Current (%1)").arg(Tools::humanReadableTimeDifference(seconds));
                }
                return Tools::humanReadableTimeDifference(seconds);
            }
            return seconds;
        }
        case 2:
            if (index.row() < m_historyModifications.size()) {
                return m_historyModifications[index.row()];
            }
            return {};
        case 3:
            if (role == Qt::DisplayRole) {
                return Tools::humanReadableFileSize(entry->size(), 0);
            }
            return entry->size();
        }
    } else if (role == Qt::FontRole && entry == m_parentEntry) {
        QFont font;
        font.setBold(true);
        return font;
    }

    return {};
}

QVariant EntryHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Last modified");
        case 1:
            return tr("Age");
        case 2:
            return tr("Difference");
        case 3:
            return tr("Size");
        }
    }

    return {};
}

void EntryHistoryModel::setEntries(const QList<Entry*>& entries, Entry* parentEntry)
{
    beginResetModel();
    m_parentEntry = parentEntry;
    m_historyEntries = entries;
    m_historyEntries << parentEntry;
    // Sort the entries by last modified (newest -> oldest) so we can calculate the differences
    std::sort(m_historyEntries.begin(), m_historyEntries.end(), [](const Entry* lhs, const Entry* rhs) {
        return lhs->timeInfo().lastModificationTime() > rhs->timeInfo().lastModificationTime();
    });
    m_deletedHistoryEntries.clear();
    calculateHistoryModifications();
    endResetModel();
}

void EntryHistoryModel::clear()
{
    beginResetModel();

    m_historyEntries.clear();
    m_deletedHistoryEntries.clear();

    endResetModel();
}

void EntryHistoryModel::clearDeletedEntries()
{
    m_deletedHistoryEntries.clear();
}

QList<Entry*> EntryHistoryModel::deletedEntries()
{
    return m_deletedHistoryEntries;
}

void EntryHistoryModel::deleteIndex(QModelIndex index)
{
    auto entry = entryFromIndex(index);
    if (entry) {
        beginRemoveRows(QModelIndex(), m_historyEntries.indexOf(entry), m_historyEntries.indexOf(entry));
        m_historyEntries.removeAll(entry);
        m_deletedHistoryEntries << entry;
        endRemoveRows();
    }
}

void EntryHistoryModel::deleteAll()
{
    Q_ASSERT(m_historyEntries.count() > 0);

    beginRemoveRows(QModelIndex(), 0, m_historyEntries.size() - 1);

    for (Entry* entry : asConst(m_historyEntries)) {
        if (entry != m_parentEntry) {
            m_deletedHistoryEntries << entry;
        }
    }
    m_historyEntries.clear();
    endRemoveRows();
}

void EntryHistoryModel::calculateHistoryModifications()
{
    m_historyModifications.clear();

    Entry* compare = nullptr;
    for (const auto curr : m_historyEntries) {
        if (!compare) {
            compare = curr;
            continue;
        }

        QStringList modifiedFields;

        if (*curr->attributes() != *compare->attributes()) {
            bool foundAttribute = false;

            if (curr->title() != compare->title()) {
                modifiedFields << tr("Title");
                foundAttribute = true;
            }
            if (curr->username() != compare->username()) {
                modifiedFields << tr("Username");
                foundAttribute = true;
            }
            if (curr->password() != compare->password()) {
                modifiedFields << tr("Password");
                foundAttribute = true;
            }
            if (curr->url() != compare->url()) {
                modifiedFields << tr("URL");
                foundAttribute = true;
            }
            if (curr->notes() != compare->notes()) {
                modifiedFields << tr("Notes");
                foundAttribute = true;
            }

            if (!foundAttribute) {
                modifiedFields << tr("Custom Attributes");
            }
        }
        if (curr->iconNumber() != compare->iconNumber() || curr->iconUuid() != compare->iconUuid()) {
            modifiedFields << tr("Icon");
        }
        if (curr->foregroundColor() != compare->foregroundColor()
            || curr->backgroundColor() != compare->backgroundColor()) {
            modifiedFields << tr("Color");
        }
        if (curr->timeInfo().expires() != compare->timeInfo().expires()
            || curr->timeInfo().expiryTime() != compare->timeInfo().expiryTime()) {
            modifiedFields << tr("Expiration");
        }
        if (curr->totp() != compare->totp()) {
            modifiedFields << tr("TOTP");
        }
        if (*curr->customData() != *compare->customData()) {
            modifiedFields << tr("Custom Data");
        }
        if (*curr->attachments() != *compare->attachments()) {
            modifiedFields << tr("Attachments");
        }
        if (*curr->autoTypeAssociations() != *compare->autoTypeAssociations()
            || curr->autoTypeEnabled() != compare->autoTypeEnabled()
            || curr->defaultAutoTypeSequence() != compare->defaultAutoTypeSequence()) {
            modifiedFields << tr("Auto-Type");
        }
        if (curr->tags() != compare->tags()) {
            modifiedFields << tr("Tags");
        }

        m_historyModifications << modifiedFields.join(", ");

        compare = curr;
    }
}
