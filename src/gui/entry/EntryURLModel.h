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

#ifndef KEEPASSXC_ENTRYURLMODEL_H
#define KEEPASSXC_ENTRYURLMODEL_H

#include "core/Entry.h"
#include <QStandardItemModel>
#include <QStyledItemDelegate>

class EntryAttributes;

class URLModelIconDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->decorationPosition = QStyleOptionViewItem::Right;
    }
};

class EntryURLModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit EntryURLModel(QObject* parent = nullptr);

    void setEntryAttributes(EntryAttributes* entryAttributes);
    void setEntryUrl(const QString& entryUrl);
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex& index, int role) const override;
    QModelIndex indexByKey(const QString& key) const;
    QString keyByIndex(const QModelIndex& index) const;

private slots:
    void updateAttributes();

private:
    QList<QPair<QString, QString>> m_urls;
    EntryAttributes* m_entryAttributes;
    QIcon m_errorIcon;
    QString m_entryUrl;
};

#endif // KEEPASSXC_ENTRYURLMODEL_H
