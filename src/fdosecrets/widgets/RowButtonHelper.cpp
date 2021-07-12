/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include <QAbstractItemView>
#include <QStyledItemDelegate>

#include <functional>

namespace
{
    class WidgetItemDelegate : public QStyledItemDelegate
    {
        std::function<QWidget*(QWidget*, const QModelIndex&)> m_create;

    public:
        explicit WidgetItemDelegate(QObject* parent, std::function<QWidget*(QWidget*, const QModelIndex&)>&& create)
            : QStyledItemDelegate(parent)
            , m_create(std::move(create))
        {
        }

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
        {
            if (!index.isValid())
                return nullptr;
            return m_create(parent, index);
        }
    };
} // namespace

void installWidgetItemDelegate(QAbstractItemView* view,
                               int column,
                               std::function<QWidget*(QWidget*, const QModelIndex&)>&& create)
{
    auto delegate = new WidgetItemDelegate(view, std::move(create));
    // doesn't take ownership
    view->setItemDelegateForColumn(column, delegate);

    for (int row = 0; row != view->model()->rowCount({}); ++row) {
        view->openPersistentEditor(view->model()->index(row, column));
    }
    QObject::connect(view->model(),
                     &QAbstractItemModel::rowsInserted,
                     delegate,
                     [view, column](const QModelIndex&, int first, int last) {
                         for (int i = first; i <= last; ++i) {
                             auto idx = view->model()->index(i, column);
                             view->openPersistentEditor(idx);
                         }
                     });
}
