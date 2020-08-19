/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "core/Entry.h"
#include "core/Resources.h"
#include "core/Tools.h"
#include "gui/styles/StateColorPalette.h"

#include <algorithm>

EntryURLModel::EntryURLModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_entryAttributes(nullptr)
    , m_errorIcon(resources()->icon("dialog-error"))
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
    const auto urlValid = Tools::checkUrlValid(value);

    if (role == Qt::BackgroundRole && !urlValid) {
        StateColorPalette statePalette;
        return statePalette.color(StateColorPalette::ColorRole::Error);
    } else if (role == Qt::DecorationRole && !urlValid) {
        return m_errorIcon;
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return value;
    } else if (role == Qt::ToolTipRole && !urlValid) {
        return tr("Invalid URL");
    }

    return {};
}

bool EntryURLModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || value.type() != QVariant::String || value.toString().isEmpty()) {
        return false;
    }

    const int row = index.row();
    const QString key = m_urls.at(row).first;
    const QString oldValue = m_urls.at(row).second;

    if (EntryAttributes::isDefaultAttribute(key) || m_entryAttributes->containsValue(value.toString())) {
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
        return QModelIndex();
    } else {
        return index(row, 0);
    }
}

QString EntryURLModel::keyByIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QString();
    } else {
        return m_urls.at(index.row()).first;
    }
}

void EntryURLModel::updateAttributes()
{
    clear();
    m_urls.clear();

    const QList<QString> attributesKeyList = m_entryAttributes->keys();
    for (const QString& key : attributesKeyList) {
        if (!EntryAttributes::isDefaultAttribute(key) && key.contains("KP2A_URL")) {
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
