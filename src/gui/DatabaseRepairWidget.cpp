/*
 *  Copyright (C) 2016 Felix Geyer <debfx@fobos.de>
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

#include "DatabaseRepairWidget.h"

#include <QFile>
#include <QFileInfo>

#include "core/Database.h"
#include "core/Metadata.h"
#include "format/KeePass2Repair.h"
#include "gui/MessageBox.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"
#include "ui_DatabaseOpenWidget.h"

DatabaseRepairWidget::DatabaseRepairWidget(QWidget* parent)
    : DatabaseOpenWidget(parent)
{
    m_ui->labelHeadline->setText(tr("Repair database"));

    connect(this, SIGNAL(editFinished(bool)), this, SLOT(processEditFinished(bool)));
}

void DatabaseRepairWidget::openDatabase()
{
    CompositeKey masterKey;

    if (m_ui->checkPassword->isChecked()) {
        masterKey.addKey(PasswordKey(m_ui->editPassword->text()));
    }

    if (m_ui->checkKeyFile->isChecked()) {
        FileKey key;
        QString keyFilename = m_ui->comboKeyFile->currentText();
        QString errorMsg;
        if (!key.load(keyFilename, &errorMsg)) {
            MessageBox::warning(this, tr("Error"), tr("Can't open key file:\n%1").arg(errorMsg));
            emit editFinished(false);
            return;
        }
        masterKey.addKey(key);
    }

    KeePass2Repair repair;

    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly)) {
        MessageBox::warning(
            this, tr("Error"), tr("Unable to open the database.").append("\n").append(file.errorString()));
        emit editFinished(false);
        return;
    }
    if (m_db) {
        delete m_db;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    auto repairOutcome = repair.repairDatabase(&file, masterKey);
    KeePass2Repair::RepairResult repairResult = repairOutcome.first;
    QApplication::restoreOverrideCursor();

    switch (repairResult) {
    case KeePass2Repair::NothingTodo:
        MessageBox::information(this, tr("Error"), tr("Database opened fine. Nothing to do."));
        emit editFinished(false);
        return;
    case KeePass2Repair::UnableToOpen:
        MessageBox::warning(
            this, tr("Error"), tr("Unable to open the database.").append("\n").append(repair.errorString()));
        emit editFinished(false);
        return;
    case KeePass2Repair::RepairSuccess:
        m_db = repairOutcome.second;
        MessageBox::warning(
            this, tr("Success"), tr("The database has been successfully repaired\nYou can now save it."));
        emit editFinished(true);
        return;
    case KeePass2Repair::RepairFailed:
        MessageBox::warning(this, tr("Error"), tr("Unable to repair the database."));
        emit editFinished(false);
        return;
    }
}

void DatabaseRepairWidget::processEditFinished(bool result)
{
    if (result) {
        emit success();
    } else {
        emit error();
    }
}
