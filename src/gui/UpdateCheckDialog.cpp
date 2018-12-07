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
#include "updatecheck/UpdateCheck.h"
#include "core/FilePath.h"

UpdateCheckDialog::UpdateCheckDialog(QWidget* parent)
: QDialog(parent)
, m_ui(new Ui::UpdateCheckDialog())
{
    m_ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->iconLabel->setPixmap(filePath()->applicationIcon().pixmap(48));

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(UpdateCheck::instance(), SIGNAL(updateCheckFinished(bool, QString)), this, SLOT(
            showUpdateCheckResponse(bool, QString)));
}

void UpdateCheckDialog::showUpdateCheckResponse(bool status, const QString& version) {
    m_ui->progressBar->setVisible(false);
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));

    if (version == QString("error")) {
        setWindowTitle(tr("Update Error!"));

        m_ui->statusLabel->setText(
                "<strong>" + tr("Update Error!") + "</strong><br><br>" +
                tr("An error occurred in retrieving update information.") + "<br>" +
                tr("Please try again later."));
        return;
    }

    if (status) {
        setWindowTitle(tr("Software Update"));
        m_ui->statusLabel->setText(
                "<strong>" + tr("A new version of KeePassXC is available!") + "</strong><br><br>" +
                tr("KeePassXC %1 is now available — you have %2.").arg(version, KEEPASSXC_VERSION) + "<br><br>" +
                "<a href='https://keepassxc.org/download/'>" +
                tr("Download it at keepassxc.org") +
                "</a>");
    } else {
        setWindowTitle(tr("You're up-to-date!"));
        m_ui->statusLabel->setText(tr(
                "KeePassXC %1 is currently the newest version available").arg(KEEPASSXC_VERSION));
    }
}

UpdateCheckDialog::~UpdateCheckDialog()
{
}
