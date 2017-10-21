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

#ifndef KEEPASSX_TOTPKEYDISPLAYDIALOG_H
#define KEEPASSX_TOTPKEYDISPLAYDIALOG_H

#include "core/Database.h"
#include "core/Entry.h"
#include "gui/DatabaseWidget.h"
#include <QDialog>
#include <QScopedPointer>
#include <QUrl>

namespace Ui
{
    class TotpKeyDisplayDialog;
}

class TotpKeyDisplayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TotpKeyDisplayDialog(DatabaseWidget* parent = nullptr, Entry* entry = nullptr);
    ~TotpKeyDisplayDialog();

private:
    QScopedPointer<Ui::TotpKeyDisplayDialog> m_ui;

private Q_SLOTS:
    void copyToClipboard();

protected:
    Entry* m_entry;
    DatabaseWidget* m_parent;

    QUrl totpKeyUri(const Entry* entry) const;
};

#endif // KEEPASSX_TOTPKEYDISPLAYDIALOG_H
