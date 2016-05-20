/*
 *  Copyright (C) 2016 Felix Geyer <debfx@fobos.de>
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

#include "ui_DatabaseOpenWidget.h"
#include "core/Database.h"
#include "core/Metadata.h"
#include "format/KeePass2Repair.h"
#include "gui/MessageBox.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

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
            MessageBox::warning(this, tr("Error"), tr("Can't open key file").append(":\n").append(errorMsg));
            Q_EMIT editFinished(false);
            return;
        }
        masterKey.addKey(key);
    }

    KeePass2Repair repair;

    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly)) {
        MessageBox::warning(this, tr("Error"), tr("Unable to open the database.").append("\n")
                            .append(file.errorString()));
        Q_EMIT editFinished(false);
        return;
    }
    if (m_db) {
        delete m_db;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    KeePass2Repair::RepairResult repairResult = repair.repairDatabase(&file, masterKey);
    QApplication::restoreOverrideCursor();

    switch (repairResult) {
    case KeePass2Repair::NothingTodo:
        MessageBox::information(this, tr("Error"), tr("Database opened fine. Nothing to do."));
        Q_EMIT editFinished(false);
        return;
    case KeePass2Repair::UnableToOpen:
        MessageBox::warning(this, tr("Error"), tr("Unable to open the database.").append("\n")
                            .append(repair.errorString()));
        Q_EMIT editFinished(false);
        return;
    case KeePass2Repair::RepairSuccess:
        m_db = repair.database();
        MessageBox::warning(this, tr("Success"), tr("The database has been successfully repaired\nYou can now save it."));
        Q_EMIT editFinished(true);
        return;
    case KeePass2Repair::RepairFailed:
        MessageBox::warning(this, tr("Error"), tr("Unable to repair the database."));
        Q_EMIT editFinished(false);
        return;
    }
}

void DatabaseRepairWidget::processEditFinished(bool result)
{
    if (result) {
        Q_EMIT success();
    }
    else {
        Q_EMIT error();
    }
}
