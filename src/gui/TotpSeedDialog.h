/*
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

#ifndef KEEPASSX_TOTPSEEDDIALOG_H
#define KEEPASSX_TOTPSEEDDIALOG_H

#include <QDialog>
#include <QScopedPointer>
#include "core/Entry.h"
#include "core/Database.h"
#include "gui/DatabaseWidget.h"

namespace Ui {
    class TotpSeedDialog;
}

class TotpSeedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TotpSeedDialog(DatabaseWidget* parent = nullptr, Entry* entry = nullptr);
    ~TotpSeedDialog();

private:
    QScopedPointer<Ui::TotpSeedDialog> m_ui;

private Q_SLOTS:
    void copyToClipboard();

protected:
    Entry* m_entry;
    DatabaseWidget* m_parent;
};

#endif // KEEPASSX_TOTPSEEDDIALOG_H
