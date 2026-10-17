/*
 * Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TagView.h"

#include "TagModel.h"
#include "core/Database.h"
#include "core/Metadata.h"
#include "gui/Application.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"

#include <QInputDialog>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>

class TagItemDelegate : public QStyledItemDelegate
{
public:
    explicit TagItemDelegate(QObject* parent)
        : QStyledItemDelegate(parent){};

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);
        if (index.data(Qt::UserRole + 1).toBool()) {
            QRect bounds = option.rect;
            bounds.setY(bounds.bottom());
            if (kpxcApp->isDarkTheme()) {
                painter->fillRect(bounds, option.palette.mid().color().lighter(185));
            } else {
                painter->fillRect(bounds, option.palette.mid());
            }
        }
    }
};

TagView::TagView(QWidget* parent)
    : QListView(parent)
    , m_model(new TagModel(this))
{
    setModel(m_model);
    setFrameStyle(QFrame::NoFrame);
    setSelectionMode(QListView::ExtendedSelection);
    setSelectionBehavior(QListView::SelectRows);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setItemDelegate(new TagItemDelegate(this));

    connect(this, &QListView::customContextMenuRequested, this, &TagView::contextMenuRequested);
}

void TagView::setDatabase(QSharedPointer<Database> db)
{
    m_db = db;
    m_model->setDatabase(db);
    setCurrentIndex(m_model->index(0));
}

void TagView::contextMenuRequested(const QPoint& pos)
{
    auto index = indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    auto type = m_model->itemType(index);
    if (type == TagModel::SAVED_SEARCH) {
        // Allow deleting saved searches
        QMenu menu;
        auto action = menu.exec({new QAction(icons()->icon("trash"), tr("Remove Search"), nullptr)}, mapToGlobal(pos));
        if (action) {
            m_db->metadata()->deleteSavedSearch(index.data(Qt::DisplayRole).toString());
        }
    } else if (type == TagModel::TAG) {
        // Allow removing and renaming tags from all entries in a database
        QMenu menu;
        auto renameAction = menu.addAction(icons()->icon("entry-edit"), tr("Rename Tag"));
        auto removeAction = menu.addAction(icons()->icon("trash"), tr("Remove Tag"));

        auto action = menu.exec(mapToGlobal(pos));
        if (action) {
            auto tag = index.data(Qt::DisplayRole).toString();
            if (action == renameAction) {
                bool ok = false;
                QString newTag = QInputDialog::getText(this,
                                                      tr("Rename Tag"),
                                                      tr("New tag name for \"%1\":").arg(tag),
                                                      QLineEdit::Normal,
                                                      tag,
                                                      &ok).trimmed();

                if (ok && newTag != tag) {
                    QString error;
                    if (!m_db->renameTag(tag, newTag, &error)) {
                        MessageBox::warning(this, tr("Error"), error);
                    }
                }
            } else if (action == removeAction) {
                auto ans = MessageBox::question(this,
                                                tr("Confirm Remove Tag"),
                                                tr("Remove tag \"%1\" from all entries in this database?").arg(tag),
                                                MessageBox::Remove | MessageBox::Cancel);
                if (ans == MessageBox::Remove) {
                    m_db->removeTag(tag);
                }
            }
        }
    }
}
