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

#ifndef KEEPASSX_HEADERVIEWSYNC_H
#define KEEPASSX_HEADERVIEWSYNC_H

#include "gui/DatabaseWidget.h"

class DatabaseWidgetStateSync : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseWidgetStateSync(QObject* parent = Q_NULLPTR);
    ~DatabaseWidgetStateSync();

public Q_SLOTS:
    void setActive(DatabaseWidget* dbWidget);
    void restoreListView();
    void restoreSearchView();

private Q_SLOTS:
    void blockUpdates();
    void updateSplitterSizes();
    void updateColumnSizes();

private:
    static QList<int> variantToIntList(const QVariant& variant);
    static QVariant intListToVariant(const QList<int>& list);

    DatabaseWidget* m_activeDbWidget;

    bool m_blockUpdates;
    QList<int> m_splitterSizes;
    QList<int> m_columnSizesList;
    QList<int> m_columnSizesSearch;
};

#endif // KEEPASSX_HEADERVIEWSYNC_H
