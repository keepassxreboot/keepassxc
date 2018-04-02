/*
*  Copyright (C) 2013 Francois Ferrand
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef BROWSERACCESSCONTROLDIALOG_H
#define BROWSERACCESSCONTROLDIALOG_H

#include <QDialog>
#include <QScopedPointer>

class Entry;

namespace Ui
{
    class BrowserAccessControlDialog;
}

class BrowserAccessControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BrowserAccessControlDialog(QWidget* parent = nullptr);
    ~BrowserAccessControlDialog();

    void setUrl(const QString& url);
    void setItems(const QList<Entry*>& items);
    bool remember() const;
    void setRemember(bool r);

private:
    QScopedPointer<Ui::BrowserAccessControlDialog> ui;
};

#endif // BROWSERACCESSCONTROLDIALOG_H
