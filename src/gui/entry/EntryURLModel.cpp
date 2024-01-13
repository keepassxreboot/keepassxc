/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "EntryURLModel.h"

#include "browser/BrowserService.h"
#include "core/EntryAttributes.h"
#include "core/UrlTools.h"
#include "gui/Icons.h"
#include "gui/styles/StateColorPalette.h"

EntryURLModel::EntryURLModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_entryAttributes(nullptr)
    , m_errorIcon(icons()->icon("dialog-error"))
{
}

void EntryURLModel::setEntryAttributes(EntryAttributes* entryAttributes)
{
    beginResetModel();

    if (m_entryAttributes) {
        m_entryAttributes->disconnect(this);
    }

    m_entryAttributes = entryAttributes;

    if (m_entryAttributes) {
        updateAttributes();
        // clang-format off
        connect(m_entryAttributes, SIGNAL(added(QString)), SLOT(updateAttributes()));
        connect(m_entryAttributes, SIGNAL(customKeyModified(QString)), SLOT(updateAttributes()));
        connect(m_entryAttributes, SIGNAL(removed(QString)), SLOT(updateAttributes()));
        connect(m_entryAttributes, SIGNAL(renamed(QString,QString)), SLOT(updateAttributes()));
        connect(m_entryAttributes, SIGNAL(reset()), SLOT(updateAttributes()));
        // clang-format on
    }

    endResetModel();
}

QVariant EntryURLModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const auto key = keyByIndex(index);
    if (key.isEmpty()) {
        return {};
    }

    const auto value = m_entryAttributes->value(key);
    const auto urlValid = urlTools()->isUrlValid(value);

    // Check for duplicate URLs in the attribute list. Excludes the current key/value from the comparison.
    auto customAttributeKeys = m_entryAttributes->customKeys().filter(EntryAttributes::AdditionalUrlAttribute);
    customAttributeKeys.removeOne(key);

    const auto duplicateUrl =
        m_entryAttributes->values(customAttributeKeys).contains(value) || urlTools()->isUrlIdentical(value, m_entryUrl);
    if (role == Qt::BackgroundRole && (!urlValid || duplicateUrl)) {
        StateColorPalette statePalette;
        return statePalette.color(StateColorPalette::ColorRole::Error);
    } else if (role == Qt::DecorationRole && (!urlValid || duplicateUrl)) {
        return m_errorIcon;
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return value;
    } else if (role == Qt::ToolTipRole && duplicateUrl) {
        return tr("Duplicate URL");
    } else if (role == Qt::ToolTipRole && !urlValid) {
        return tr("Invalid URL");
    }

    return {};
}

void EntryURLModel::setEntryUrl(const QString& entryUrl)
{
    m_entryUrl = entryUrl;
}

bool EntryURLModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || value.type() != QVariant::String || value.toString().isEmpty()) {
        return false;
    }

    const auto row = index.row();
    const auto key = m_urls.at(row).first;

    if (EntryAttributes::isDefaultAttribute(key)) {
        return false;
    }

    m_entryAttributes->set(key, value.toString());

    emit dataChanged(this->index(row, 0), this->index(row, columnCount() - 1));
    return true;
}

QModelIndex EntryURLModel::indexByKey(const QString& key) const
{
    int row = -1;
    for (int i = 0; i < m_urls.size(); ++i) {
        if (m_urls.at(i).first == key) {
            row = i;
            break;
        }
    }

    if (row == -1) {
        return {};
    } else {
        return index(row, 0);
    }
}

QString EntryURLModel::keyByIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    } else {
        return m_urls.at(index.row()).first;
    }
}

void EntryURLModel::updateAttributes()
{
    clear();
    m_urls.clear();

    const auto attributesKeyList = m_entryAttributes->keys();
    for (const auto& key : attributesKeyList) {
        if (!EntryAttributes::isDefaultAttribute(key) && key.contains(EntryAttributes::AdditionalUrlAttribute)) {
            const auto value = m_entryAttributes->value(key);
            m_urls.append(qMakePair(key, value));

            auto* item = new QStandardItem(value);
            if (m_entryAttributes->isProtected(key)) {
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            }
            appendRow(item);
        }
    }
}
