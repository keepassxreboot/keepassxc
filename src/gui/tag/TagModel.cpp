/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "TagModel.h"

#include "core/Database.h"
#include "gui/Icons.h"

TagModel::TagModel(QSharedPointer<Database> db, QObject* parent)
    : QAbstractListModel(parent)
{
    setDatabase(db);
}

TagModel::~TagModel()
{
}

void TagModel::setDatabase(QSharedPointer<Database> db)
{
    m_db = db;
    if (!m_db) {
        m_tagList.clear();
        return;
    }
    connect(m_db.data(), SIGNAL(tagListUpdated()), SLOT(updateTagList()));
    updateTagList();
}

void TagModel::updateTagList()
{
    beginResetModel();
    m_tagList.clear();
    m_tagList << tr("All") << tr("Expired") << tr("Weak Passwords") << m_db->tagList();
    endResetModel();
}

int TagModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_tagList.size();
}

QVariant TagModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_tagList.size()) {
        return {};
    }

    switch (role) {
    case Qt::DecorationRole:
        if (index.row() <= 2) {
            return icons()->icon("tag-search");
        }
        return icons()->icon("tag");
    case Qt::DisplayRole:
        return m_tagList.at(index.row());
    case Qt::UserRole:
        if (index.row() == 0) {
            return "";
        } else if (index.row() == 1) {
            return "is:expired";
        } else if (index.row() == 2) {
            return "is:weak";
        }
        return QString("tag:%1").arg(m_tagList.at(index.row()));
    }

    return {};
}

const QStringList& TagModel::tags() const
{
    return m_tagList;
}
