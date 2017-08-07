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
    m_splitterSizes = variantToIntList(config()->get("GUI/SplitterState"));
    /**
     * @author Fonic <https://github.com/fonic>
     * Load entry list/search header state
     */
    m_headerStateList = variantToByteArray(config()->get("GUI/EntryListHeaderState"));
    m_headerStateSearch = variantToByteArray(config()->get("GUI/EntrySearchHeaderState"));
}

DatabaseWidgetStateSync::~DatabaseWidgetStateSync()
{
    config()->set("GUI/SplitterState", intListToVariant(m_splitterSizes));
    /**
     * @author Fonic <https://github.com/fonic>
     * Save entry list/search header state
     */
    config()->set("GUI/EntryListHeaderState", byteArrayToVariant(m_headerStateList));
    config()->set("GUI/EntrySearchHeaderState", byteArrayToVariant(m_headerStateSearch));
}

void DatabaseWidgetStateSync::setActive(DatabaseWidget* dbWidget)
{
    if (m_activeDbWidget) {
        disconnect(m_activeDbWidget, 0, this, 0);
    }

    m_activeDbWidget = dbWidget;

    if (m_activeDbWidget) {
        m_blockUpdates = true;

        if (!m_splitterSizes.isEmpty())
            m_activeDbWidget->setSplitterSizes(m_splitterSizes);

        if (m_activeDbWidget->isInSearchMode())
            restoreSearchView();
        else
            restoreListView();

        m_blockUpdates = false;

        connect(m_activeDbWidget, SIGNAL(splitterSizesChanged()),
                SLOT(updateSplitterSizes()));
        /**
         * @author Fonic <https://github.com/fonic>
         * Receive to receive entry view header state changes
         */
        connect(m_activeDbWidget, SIGNAL(entryViewHeaderStateChanged()),
                SLOT(updateHeaderStates()));
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
    /**
     * @author Fonic <https://github.com/fonic>
     * Apply entry list header state to widget
     */
    if (!m_headerStateList.isEmpty()) {
        m_activeDbWidget->setEntryViewHeaderState(m_headerStateList);
    }

    m_blockUpdates = false;
}

void DatabaseWidgetStateSync::restoreSearchView()
{
    /**
     * @author Fonic <https://github.com/fonic>
     * Apply entry search header state to widget
     */
    if (!m_headerStateSearch.isEmpty()) {
        m_activeDbWidget->setEntryViewHeaderState(m_headerStateSearch);
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

void DatabaseWidgetStateSync::updateHeaderStates()
{
    if (m_blockUpdates) {
        return;
    }

    /**
     * @author Fonic <https://github.com/fonic>
     * Retrieve entry list/search header state from widget
     */
    if (m_activeDbWidget->isGroupSelected()) {
        m_headerStateList = m_activeDbWidget->entryViewHeaderState();
    }
    else {
        m_headerStateSearch = m_activeDbWidget->entryViewHeaderState();
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

/**
 * @author Fonic <https://github.com/fonic>
 * Method to convert QVariant to QByteArray
 */
QByteArray DatabaseWidgetStateSync::variantToByteArray(const QVariant& variant)
{
    if (variant.canConvert<QByteArray>())
        return variant.toByteArray();
    else
        return QByteArray();
}

/**
 * @author Fonic <https://github.com/fonic>
 * Method to convert QByteArray from QVariant (trivial, just put here for the
 * sake of completeness)
 */
QVariant DatabaseWidgetStateSync::byteArrayToVariant(const QByteArray& bytearray) {
    return QVariant(bytearray);
}
