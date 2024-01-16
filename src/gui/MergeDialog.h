/*
 *  Copyright (C) 2017 Weslly Honorato <﻿weslly@protonmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_MERGEDIALOG_H
#define KEEPASSX_MERGEDIALOG_H

#include "core/Merger.h"
#include "ui_MergeDialog.h"

#include <QDialog>

class Database;

class MergeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MergeDialog(QSharedPointer<Database> source, QSharedPointer<Database> target, QWidget* parent = nullptr);

Q_SIGNALS:
    void databaseMerged(bool databaseChanged);

private Q_SLOTS:
    void performMerge();

private:
    void setupChangeTable();

private:
    Ui::MergeDialog m_ui;

    Merger m_merger;
    QSharedPointer<Database> m_sourceDatabase;
    QSharedPointer<Database> m_targetDatabase;
};

#endif // KEEPASSX_MERGEDIALOG_H
