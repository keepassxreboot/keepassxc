/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_PASSKEYIMPORTDIALOG_H
#define KEEPASSXC_PASSKEYIMPORTDIALOG_H

#include "core/Database.h"
#include "core/Group.h"
#include <QDialog>
#include <QUuid>

namespace Ui
{
    class PasskeyImportDialog;
}

class PasskeyImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasskeyImportDialog(QWidget* parent = nullptr);
    ~PasskeyImportDialog() override;

    void setInfo(const QString& relyingParty,
                 const QString& username,
                 const QSharedPointer<Database>& database,
                 bool isEntry);
    QSharedPointer<Database> getSelectedDatabase() const;
    QUuid getSelectedEntryUuid() const;
    QUuid getSelectedGroupUuid() const;
    bool useDefaultGroup() const;
    bool createNewEntry() const;

private:
    void addDatabases();

signals:
    void updateEntries();
    void updateGroups();

private slots:
    void addEntries();
    void addGroups();
    void changeDatabase(int index);
    void changeEntry(int index);
    void changeGroup(int index);

private:
    QScopedPointer<Ui::PasskeyImportDialog> m_ui;
    QSharedPointer<Database> m_selectedDatabase;
    QUuid m_selectedDatabaseUuid;
    QUuid m_selectedEntryUuid;
    QUuid m_selectedGroupUuid;
};

#endif // KEEPASSXC_PASSKEYIMPORTDIALOG_H
