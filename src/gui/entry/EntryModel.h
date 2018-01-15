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

#ifndef KEEPASSX_ENTRYMODEL_H
#define KEEPASSX_ENTRYMODEL_H

#include <QAbstractTableModel>

class Entry;
class Group;

class EntryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @author Fonic <https://github.com/fonic>
     * Add entries for additional columns 'Password', 'Notes', 'Expires',
     * 'Created', 'Modified', 'Accessed' and 'Attachments'
     */
    enum ModelColumn
    {
        ParentGroup = 0,
        Title = 1,
        Username = 2,
        Password = 3,
        Url = 4,
        Notes = 5,
        Expires = 6,
        Created = 7,
        Modified = 8,
        Accessed = 9,
        Paperclip = 10,
        Attachments = 11
    };

    explicit EntryModel(QObject* parent = nullptr);
    Entry* entryFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromEntry(Entry* entry) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;
    Qt::ItemFlags flags(const QModelIndex& modelIndex) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    void setEntryList(const QList<Entry*>& entries);

    /**
     * @author Fonic <https://github.com/fonic>
     * Methods to get/set state of 'Hide Usernames' and 'Hide Passwords'
     * settings
     */
    bool hideUsernames() const;
    void setHideUsernames(const bool hide);
    bool hidePasswords() const;
    void setHidePasswords(const bool hide);

signals:
    /**
     * @author Fonic <https://github.com/fonic>
     * Signals to notify about list/search mode switches (NOTE: previously
     * named 'switchedToGroupMode'/'switchedToEntryListMode')
     */
    void switchedToListMode();
    void switchedToSearchMode();

    /**
     * @author Fonic <https://github.com/fonic>
     * Signals to notify about state changes of 'Hide Usernames' and 'Hide
     * Passwords' settings
     */
    void hideUsernamesChanged();
    void hidePasswordsChanged();

public slots:
    void setGroup(Group* group);

    /**
     * @author Fonic <https://github.com/fonic>
     * Slots to toggle state of 'Hide Usernames' and 'Hide Passwords' settings
     */
    void toggleHideUsernames(const bool hide);
    void toggleHidePasswords(const bool hide);

private slots:
    void entryAboutToAdd(Entry* entry);
    void entryAdded(Entry* entry);
    void entryAboutToRemove(Entry* entry);
    void entryRemoved();
    void entryDataChanged(Entry* entry);

private:
    void severConnections();
    void makeConnections(const Group* group);

    Group* m_group;
    QList<Entry*> m_entries;
    QList<Entry*> m_orgEntries;
    QList<const Group*> m_allGroups;

    /**
     * @author Fonic <https://github.com/fonic>
     * Properties to store state of 'Hide Usernames' and 'Hide Passwords'
     * settings
     */
    bool m_hideUsernames;
    bool m_hidePasswords;

    /**
     * @author Fonic <https://github.com/fonic>
     * Constant string used to display hidden content in columns 'Username'
     * and 'Password'
     */
    static const QString HiddenContentDisplay;

    /**
     * @author Fonic <https://github.com/fonic>
     * Date format used to display dates in columns 'Expires', 'Created',
     * 'Modified' and 'Accessed'
     */
    static const Qt::DateFormat DateFormat;

    /**
     * @author Fonic <https://github.com/fonic>
     * Constant string used to display header and data of column 'Paper-
     * clip'
     */
    static const QString PaperClipDisplay;
};

#endif // KEEPASSX_ENTRYMODEL_H
