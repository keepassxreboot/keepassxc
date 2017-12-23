/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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

#ifndef KEEPASSX_DATABASEWIDGETSTATESYNC_H
#define KEEPASSX_DATABASEWIDGETSTATESYNC_H

#include "gui/DatabaseWidget.h"

class DatabaseWidgetStateSync : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseWidgetStateSync(QObject* parent = nullptr);
    ~DatabaseWidgetStateSync();

public slots:
    void setActive(DatabaseWidget* dbWidget);
    void restoreListView();
    void restoreSearchView();

private slots:
    void blockUpdates();
    void updateSplitterSizes();

    /**
     * @author Fonic <https://github.com/fonic>
     * Slot to update entry view view state
     */
    void updateViewState();

private:
    static QList<int> variantToIntList(const QVariant& variant);
    static QVariant intListToVariant(const QList<int>& list);

    DatabaseWidget* m_activeDbWidget;

    bool m_blockUpdates;
    QList<int> m_mainSplitterSizes;
    QList<int> m_detailSplitterSizes;

    /**
     * @author Fonic <https://github.com/fonic>
     * Properties to store state of entry view 'Hide Usernames'/'Hide
     * Passwords' settings
     */
    bool m_entryHideUsernames;
    bool m_entryHidePasswords;

    /**
     * @author Fonic <https://github.com/fonic>
     * Properties to store states of entry view list/search view (replaces
     * m_columnSizesList/m_columnSizesSearch)
     */
    QByteArray m_entryListViewState;
    QByteArray m_entrySearchViewState;
};

#endif // KEEPASSX_DATABASEWIDGETSTATESYNC_H
