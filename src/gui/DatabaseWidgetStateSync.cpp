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

    /**
     * @author Fonic <https://github.com/fonic>
     * Load state of entry view 'Hide Usernames'/'Hide Passwords' settings
     */
    m_entryHideUsernames = config()->get("GUI/EntryHideUsernames").toBool();
    m_entryHidePasswords = config()->get("GUI/EntryHidePasswords").toBool();

    /**
     * @author Fonic <https://github.com/fonic>
     * Load states of entry view list/search view
     */
    m_entryListViewState = config()->get("GUI/EntryListViewState").toByteArray();
    m_entrySearchViewState = config()->get("GUI/EntrySearchViewState").toByteArray();
}

DatabaseWidgetStateSync::~DatabaseWidgetStateSync()
{
    config()->set("GUI/SplitterState", intListToVariant(m_mainSplitterSizes));
    config()->set("GUI/DetailSplitterState", intListToVariant(m_detailSplitterSizes));

    /**
     * @author Fonic <https://github.com/fonic>
     * Save state of entry view 'Hide Usernames'/'Hide Passwords' settings
     */
    config()->set("GUI/EntryHideUsernames", m_entryHideUsernames);
    config()->set("GUI/EntryHidePasswords", m_entryHidePasswords);

    /**
     * @author Fonic <https://github.com/fonic>
     * Save states of entry view list/search view
     */
    config()->set("GUI/EntryListViewState", m_entryListViewState);
    config()->set("GUI/EntrySearchViewState", m_entrySearchViewState);
}

void DatabaseWidgetStateSync::setActive(DatabaseWidget* dbWidget)
{
    if (m_activeDbWidget) {
        disconnect(m_activeDbWidget, 0, this, 0);
    }

    m_activeDbWidget = dbWidget;

    if (m_activeDbWidget) {
        m_blockUpdates = true;

        if (!m_mainSplitterSizes.isEmpty()) {
            m_activeDbWidget->setMainSplitterSizes(m_mainSplitterSizes);
        }

        if (!m_detailSplitterSizes.isEmpty()) {
            m_activeDbWidget->setDetailSplitterSizes(m_detailSplitterSizes);
        }

        if (m_activeDbWidget->isInSearchMode()) {
            restoreSearchView();
        } else {
            restoreListView();
        }

        m_blockUpdates = false;

        connect(m_activeDbWidget, SIGNAL(mainSplitterSizesChanged()),
                SLOT(updateSplitterSizes()));
        connect(m_activeDbWidget, SIGNAL(detailSplitterSizesChanged()),
                SLOT(updateSplitterSizes()));

        /**
         * @author Fonic <https://github.com/fonic>
         * Connect signal to receive state changes of entry view view
         */
        connect(m_activeDbWidget, SIGNAL(entryViewStateChanged()),
                SLOT(updateViewState()));

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

/**
 * @author Fonic <https://github.com/fonic>
 * Restore entry view list view state
 *
 * NOTE:
 * States of entry view 'Hide Usernames'/'Hide Passwords' settings are considered
 * 'global', i.e. they are the same for both list and search mode
 *
 * NOTE:
 * If m_entryListViewState is empty, it is the first time after clean/invalid
 * config that list view is activated. Thus, save its current state. Without
 * this, m_entryListViewState would remain empty until there is an actual view
 * state change (e.g. column is resized)
 */
void DatabaseWidgetStateSync::restoreListView()
{
    m_activeDbWidget->setEntryViewHideUsernames(m_entryHideUsernames);
    m_activeDbWidget->setEntryViewHidePasswords(m_entryHidePasswords);

    if (!m_entryListViewState.isEmpty()) {
        m_activeDbWidget->setEntryViewViewState(m_entryListViewState);
    }
    else {
        m_entryListViewState = m_activeDbWidget->entryViewViewState();
    }

    m_blockUpdates = false;
}

/**
 * @author Fonic <https://github.com/fonic>
 * Restore entry view search view state
 *
 * NOTE:
 * States of entry view 'Hide Usernames'/'Hide Passwords' settings are considered
 * 'global', i.e. they are the same for both list and search mode
 *
 * NOTE:
 * If m_entrySearchViewState is empty, it is the first time after clean/invalid
 * config that search view is activated. Thus, save its current state. Without
 * this, m_entrySearchViewState would remain empty until there is an actual view
 * state change (e.g. column is resized)
 */
void DatabaseWidgetStateSync::restoreSearchView()
{
    m_activeDbWidget->setEntryViewHideUsernames(m_entryHideUsernames);
    m_activeDbWidget->setEntryViewHidePasswords(m_entryHidePasswords);

    if (!m_entrySearchViewState.isEmpty()) {
        m_activeDbWidget->setEntryViewViewState(m_entrySearchViewState);
    }
    else {
        m_entrySearchViewState = m_activeDbWidget->entryViewViewState();
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

/**
 * @author Fonic <https://github.com/fonic>
 * Update entry view list/search view state (NOTE: states of entry view
 * 'Hide Usernames'/'Hide Passwords' settings are considered 'global',
 * i.e. they are the same for both list and search mode)
 */
void DatabaseWidgetStateSync::updateViewState()
{
    if (m_blockUpdates) {
        return;
    }

    m_entryHideUsernames = m_activeDbWidget->entryViewHideUsernames();
    m_entryHidePasswords = m_activeDbWidget->entryViewHidePasswords();

    if (m_activeDbWidget->isInSearchMode()) {
        m_entrySearchViewState = m_activeDbWidget->entryViewViewState();
    }
    else {
        m_entryListViewState = m_activeDbWidget->entryViewViewState();
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

