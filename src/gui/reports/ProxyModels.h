/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_PROXYMODELS_H
#define KEEPASSXC_PROXYMODELS_H

#include <QModelIndex>
#include <QSortFilterProxyModel>

enum class SortProxyModelKind
{
    Default = 0,
    Hibp,
    Healthcheck,
};

class HibpReportSortProxyModel : public QSortFilterProxyModel
{
public:
    HibpReportSortProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }
    ~HibpReportSortProxyModel() override = default;

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        // Sort count column by user data
        if (left.column() == 2) {
            return sourceModel()->data(left, Qt::UserRole).toInt() < sourceModel()->data(right, Qt::UserRole).toInt();
        }
        // Otherwise use default sorting
        return QSortFilterProxyModel::lessThan(left, right);
    }
};

class HealthcheckReportSortProxyModel : public QSortFilterProxyModel
{
public:
    HealthcheckReportSortProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }
    ~HealthcheckReportSortProxyModel() override = default;

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        // Check if the display data is a number, convert and compare if so
        bool ok = false;
        int leftInt = sourceModel()->data(left).toString().toInt(&ok);
        if (ok) {
            return leftInt < sourceModel()->data(right).toString().toInt();
        }
        // Otherwise use default sorting
        return QSortFilterProxyModel::lessThan(left, right);
    }
};

#endif // KEEPASSXC_PROXYMODELS_H
