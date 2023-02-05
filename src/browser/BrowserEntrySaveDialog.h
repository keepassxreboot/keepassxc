/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_BROWSERENTRYSAVEDIALOG_H
#define KEEPASSXC_BROWSERENTRYSAVEDIALOG_H

#include "gui/DatabaseTabWidget.h"

class QListWidgetItem;

class Entry;

namespace Ui
{
    class BrowserEntrySaveDialog;
}

class BrowserEntrySaveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BrowserEntrySaveDialog(QWidget* parent = nullptr);
    ~BrowserEntrySaveDialog() override;

    int setItems(QList<DatabaseWidget*>& databaseWidgets, DatabaseWidget* currentWidget) const;
    QList<QListWidgetItem*> getSelected() const;

private:
    QScopedPointer<Ui::BrowserEntrySaveDialog> m_ui;
};

#endif // KEEPASSXC_BROWSERENTRYSAVEDIALOG_H
