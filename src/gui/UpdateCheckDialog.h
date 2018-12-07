/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_UPDATECHECKDIALOG_H
#define KEEPASSXC_UPDATECHECKDIALOG_H

#include <QUrl>
#include <QDialog>
#include <QScopedPointer>
#include "gui/MessageBox.h"
#include "config-keepassx.h"
#include "core/Global.h"
#include "updatecheck/UpdateCheck.h"

namespace Ui
{
    class UpdateCheckDialog;
}

class UpdateCheckDialog : public QDialog
{
Q_OBJECT

public:
    explicit UpdateCheckDialog(QWidget* parent = nullptr);
    ~UpdateCheckDialog() override;

public slots:
    void showUpdateCheckResponse(bool status, const QString &version);

private:
    QScopedPointer<Ui::UpdateCheckDialog> m_ui;
};


#endif //KEEPASSXC_UPDATECHECKDIALOG_H
