/*
*  Copyright (C) 2013 Francois Ferrand
*  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
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

#ifndef BROWSEROPTIONDIALOG_H
#define BROWSEROPTIONDIALOG_H

#include <QWidget>
#include <QScopedPointer>

namespace Ui {
class BrowserOptionDialog;
}

class BrowserOptionDialog : public QWidget
{
    Q_OBJECT

public:
    explicit BrowserOptionDialog(QWidget* parent = nullptr);
    ~BrowserOptionDialog();

public slots:
    void loadSettings();
    void saveSettings();

signals:
    void removeSharedEncryptionKeys();
    void removeStoredPermissions();

private slots:
    void showProxyLocationFileDialog();

private:
    QScopedPointer<Ui::BrowserOptionDialog> m_ui;
};

#endif // BROWSEROPTIONDIALOG_H
