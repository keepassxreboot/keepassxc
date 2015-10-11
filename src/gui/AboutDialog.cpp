/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "config-keepassx.h"
#include "version.h"
#include "core/FilePath.h"
#include "crypto/Crypto.h"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::AboutDialog())
{
    m_ui->setupUi(this);

    m_ui->nameLabel->setText(m_ui->nameLabel->text() + " " + KEEPASSX_VERSION);
    QFont nameLabelFont = m_ui->nameLabel->font();
    nameLabelFont.setBold(true);
    nameLabelFont.setPointSize(nameLabelFont.pointSize() + 4);
    m_ui->nameLabel->setFont(nameLabelFont);

    m_ui->iconLabel->setPixmap(filePath()->applicationIcon().pixmap(48));

    QString commitHash;
    if (!QString(GIT_HEAD).isEmpty()) {
        commitHash = GIT_HEAD;
    }
    else if (!QString(DIST_HASH).contains("Format")) {
        commitHash = DIST_HASH;
    }

    if (!commitHash.isEmpty()) {
        QString labelText = tr("Revision").append(": ").append(commitHash);
        m_ui->label_git->setText(labelText);
    }

    QString libs = QString("%1\n- Qt %2\n- %3")
            .arg(m_ui->label_libs->text())
            .arg(QString::fromLocal8Bit(qVersion()))
            .arg(Crypto::backendVersion());
    m_ui->label_libs->setText(libs);

    setAttribute(Qt::WA_DeleteOnClose);
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
}

AboutDialog::~AboutDialog()
{
}
