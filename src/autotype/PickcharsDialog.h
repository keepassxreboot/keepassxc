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

#ifndef KEEPASSXC_PICKCHARSDIALOG_H
#define KEEPASSXC_PICKCHARSDIALOG_H

#include <QDialog>

namespace Ui
{
    class PickcharsDialog;
}

class PickcharsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PickcharsDialog(const QString& string, QWidget* parent = nullptr);
    QString selectedChars();
    bool pressTab();

protected:
    void showEvent(QShowEvent*) override;

private slots:
    void charSelected();
    void upPressed();
    void downPressed();

private:
    QSharedPointer<Ui::PickcharsDialog> m_ui;
    int m_lastSelected;
};

#endif // KEEPASSXC_PICKCHARSDIALOG_H
