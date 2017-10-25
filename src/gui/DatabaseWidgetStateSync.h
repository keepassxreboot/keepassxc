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

/**
 * @author Fonic <https://github.com/fonic>
 * Added includes
 */
#include <QByteArray>

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
     * Slot to receive entry view header state changes
     */
    void updateHeaderStates();
    /**
     * @author Fonic <https://github.com/fonic>
     * Slots to receive changes of 'Hide Usernames' and 'Hide Passwords'
     * settings
     */
    void updateHideUsernames();
    void updateHidePasswords();

private:
    static QList<int> variantToIntList(const QVariant& variant);
    static QVariant intListToVariant(const QList<int>& list);
    /**
     * @author Fonic <https://github.com/fonic>
     * Methods to convert QVariant from/to QByteArray (required to save/load
     * entry list/search header state to/from configuration file)
     */
    static QByteArray variantToByteArray(const QVariant& variant);
    static QVariant byteArrayToVariant(const QByteArray& bytearray);

    DatabaseWidget* m_activeDbWidget;

    bool m_blockUpdates;
    QList<int> m_splitterSizes;
    /**
     * @author Fonic <https://github.com/fonic>
     * Properties to store entry list/search header states (replaces both
     * m_columnSizesList and m_columnSizesSearch)
     */
    QByteArray m_headerStateList;
    QByteArray m_headerStateSearch;
    /**
     * @author Fonic <https://github.com/fonic>
     * Properties to store entry list 'Hide Usernames' and 'Hide Passwords'
     * settings
     */
    bool m_hideUsernames;
    bool m_hidePasswords;
};

#endif // KEEPASSX_DATABASEWIDGETSTATESYNC_H
