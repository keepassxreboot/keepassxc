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

#include "UpdateCheckDialog.h"
#include "ui_UpdateCheckDialog.h"

#include <QPushButton>

#include "config-keepassx.h"
#include "gui/Icons.h"
#include "updatecheck/UpdateChecker.h"

UpdateCheckDialog::UpdateCheckDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::UpdateCheckDialog())
{
    m_ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->iconLabel->setPixmap(icons()->applicationIcon().pixmap(48));

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(UpdateChecker::instance(),
            SIGNAL(updateCheckFinished(bool, QString, bool)),
            SLOT(showUpdateCheckResponse(bool, QString)));
}

void UpdateCheckDialog::showUpdateCheckResponse(bool hasUpdate, const QString& version)
{
    m_ui->progressBar->setVisible(false);
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));

    setWindowTitle(tr("Software Update"));

    if (version == UpdateChecker::ErrorVersion) {
        m_ui->statusLabel->setText(
            tr("An error occurred when trying to retrieve update information, please try again later."));
    } else if (hasUpdate) {
        m_ui->statusLabel->setText(
            tr("<strong>A new version is available.</strong><br/>"
               "KeePassXC %1 can be <a href=\"https://keepassxc.org/download/\">downloaded here</a>.")
                .arg(version, KEEPASSXC_VERSION));
    } else {
        m_ui->statusLabel->setText(tr("You have the latest version of KeePassXC"));
    }
}

UpdateCheckDialog::~UpdateCheckDialog()
{
}
