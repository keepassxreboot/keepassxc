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

#include "DatabaseWidgetStateSync.h"

#include "core/Config.h"

DatabaseWidgetStateSync::DatabaseWidgetStateSync(QObject* parent)
    : QObject(parent)
    , m_activeDbWidget(Q_NULLPTR)
    , m_blockUpdates(false)
{
    m_splitterSizes = variantToIntList(config()->get("GUI/SplitterState"));
    m_columnSizesList = variantToIntList(config()->get("GUI/EntryListColumnSizes"));
    m_columnSizesSearch = variantToIntList(config()->get("GUI/EntrySearchColumnSizes"));
}

DatabaseWidgetStateSync::~DatabaseWidgetStateSync()
{
    config()->set("GUI/SplitterState", intListToVariant(m_splitterSizes));
    config()->set("GUI/EntryListColumnSizes", intListToVariant(m_columnSizesList));
    config()->set("GUI/EntrySearchColumnSizes", intListToVariant(m_columnSizesSearch));
}

void DatabaseWidgetStateSync::setActive(DatabaseWidget* dbWidget)
{
    if (m_activeDbWidget) {
        disconnect(m_activeDbWidget, 0, this, 0);
    }

    m_activeDbWidget = dbWidget;

    if (m_activeDbWidget) {
        m_blockUpdates = true;

        if (!m_splitterSizes.isEmpty()) {
            m_activeDbWidget->setSplitterSizes(m_splitterSizes);
        }

        if (m_activeDbWidget->isGroupSelected()) {
            restoreListView();
        }
        else {
            restoreSearchView();
        }

        m_blockUpdates = false;

        connect(m_activeDbWidget, SIGNAL(splitterSizesChanged()),
                SLOT(updateSplitterSizes()));
        connect(m_activeDbWidget, SIGNAL(entryColumnSizesChanged()),
                SLOT(updateColumnSizes()));
        connect(m_activeDbWidget, SIGNAL(listModeActivated()),
                SLOT(restoreListView()));
        connect(m_activeDbWidget, SIGNAL(searchModeActivated()),
                SLOT(restoreSearchView()));
        connect(m_activeDbWidget, SIGNAL(listModeAboutToActivate()),
                SLOT(blockUpdates()));
        connect(m_activeDbWidget, SIGNAL(searchModeAboutToActivate()),
                SLOT(blockUpdates()));
    }
}

void DatabaseWidgetStateSync::restoreListView()
{
    if (!m_columnSizesList.isEmpty()) {
        m_activeDbWidget->setEntryViewHeaderSizes(m_columnSizesList);
    }

    m_blockUpdates = false;
}

void DatabaseWidgetStateSync::restoreSearchView()
{
    if (!m_columnSizesSearch.isEmpty()) {
        m_activeDbWidget->setEntryViewHeaderSizes(m_columnSizesSearch);
    }

    m_blockUpdates = false;
}

void DatabaseWidgetStateSync::blockUpdates()
{
    m_blockUpdates = true;
}

void DatabaseWidgetStateSync::updateSplitterSizes()
{
    if (m_blockUpdates) {
        return;
    }

    m_splitterSizes = m_activeDbWidget->splitterSizes();
}

void DatabaseWidgetStateSync::updateColumnSizes()
{
    if (m_blockUpdates) {
        return;
    }

    if (m_activeDbWidget->isGroupSelected()) {
        m_columnSizesList = m_activeDbWidget->entryHeaderViewSizes();
    }
    else {
        m_columnSizesSearch = m_activeDbWidget->entryHeaderViewSizes();
    }
}

QList<int> DatabaseWidgetStateSync::variantToIntList(const QVariant& variant)
{
    QVariantList list = variant.toList();
    QList<int> result;

    Q_FOREACH (const QVariant& var, list) {
        bool ok;
        int size = var.toInt(&ok);
        if (ok) {
            result.append(size);
        }
        else {
            result.clear();
            break;
        }
    }

    return result;
}

QVariant DatabaseWidgetStateSync::intListToVariant(const QList<int>& list)
{
    QVariantList result;

    Q_FOREACH (int value, list) {
        result.append(value);
    }

    return result;
}
