/*
 *  Copyright (C) 2021 Team KeePassXC <team@keepassxc.org>
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

#ifndef KEEPASSXC_OPENSSHKEYGENDIALOG_H
#define KEEPASSXC_OPENSSHKEYGENDIALOG_H

#include <QDialog>
class OpenSSHKey;

namespace Ui
{
    class OpenSSHKeyGenDialog;
}

class OpenSSHKeyGenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenSSHKeyGenDialog(QWidget* parent = nullptr);
    ~OpenSSHKeyGenDialog() override;

    void accept() override;
    void setKey(OpenSSHKey* key);

private slots:
    void typeChanged();

private:
    QScopedPointer<Ui::OpenSSHKeyGenDialog> m_ui;
    OpenSSHKey* m_key;
};

#endif // KEEPASSXC_OPENSSHKEYGENDIALOG_H
