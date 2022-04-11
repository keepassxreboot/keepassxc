/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_ABOUTDIALOG_H
#define KEEPASSX_ABOUTDIALOG_H

#include <QDialog>

namespace Ui
{
    class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override;

protected slots:
    void copyToClipboard();

private:
    QScopedPointer<Ui::AboutDialog> m_ui;
};

#endif // KEEPASSX_ABOUTDIALOG_H
