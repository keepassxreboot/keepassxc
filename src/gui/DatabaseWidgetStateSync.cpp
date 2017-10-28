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
    , m_activeDbWidget(nullptr)
    , m_blockUpdates(false)
{
    m_mainSplitterSizes = variantToIntList(config()->get("GUI/SplitterState"));
    m_detailSplitterSizes = variantToIntList(config()->get("GUI/DetailSplitterState"));
    m_columnSizesList = variantToIntList(config()->get("GUI/EntryListColumnSizes"));
    m_columnSizesSearch = variantToIntList(config()->get("GUI/EntrySearchColumnSizes"));
}

DatabaseWidgetStateSync::~DatabaseWidgetStateSync()
{
    config()->set("GUI/SplitterState", intListToVariant(m_mainSplitterSizes));
    config()->set("GUI/DetailSplitterState", intListToVariant(m_detailSplitterSizes));
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

        if (!m_mainSplitterSizes.isEmpty())
            m_activeDbWidget->setMainSplitterSizes(m_mainSplitterSizes);

        if (!m_detailSplitterSizes.isEmpty())
            m_activeDbWidget->setDetailSplitterSizes(m_detailSplitterSizes);

        if (m_activeDbWidget->isInSearchMode())
            restoreSearchView();
        else
            restoreListView();

        m_blockUpdates = false;

        connect(m_activeDbWidget, SIGNAL(mainSplitterSizesChanged()),
                SLOT(updateSplitterSizes()));
        connect(m_activeDbWidget, SIGNAL(detailSplitterSizesChanged()),
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

    m_mainSplitterSizes = m_activeDbWidget->mainSplitterSizes();
    m_detailSplitterSizes = m_activeDbWidget->detailSplitterSizes();
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
    const QVariantList list = variant.toList();
    QList<int> result;

    for (const QVariant& var : list) {
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

    for (int value : list) {
        result.append(value);
    }

    return result;
}

