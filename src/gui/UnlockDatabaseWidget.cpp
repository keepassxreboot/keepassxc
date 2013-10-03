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

#include "UnlockDatabaseWidget.h"

#include <QMessageBox>

#include "ui_DatabaseOpenWidget.h"
#include "core/Database.h"

UnlockDatabaseWidget::UnlockDatabaseWidget(QWidget* parent)
    : DatabaseOpenWidget(parent)
{
    m_ui->labelHeadline->setText(tr("Unlock database"));

    m_ui->buttonBox->removeButton(m_ui->buttonBox->button(QDialogButtonBox::Cancel));
    connect(this, SIGNAL(editFinished(bool)), SLOT(clearForms()));
}

void UnlockDatabaseWidget::load(const QString& filename, Database* db)
{
    Q_ASSERT(db);

    DatabaseOpenWidget::load(filename);
    m_db = db;
}

void UnlockDatabaseWidget::openDatabase()
{
    CompositeKey masterKey = databaseKey();
    if (masterKey.isEmpty()) {
        return;
    }

    if (m_db->verifyKey(masterKey)) {
        Q_EMIT editFinished(true);
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("Wrong key."));
        m_ui->editPassword->clear();
    }
}

void UnlockDatabaseWidget::clearForms()
{
    m_ui->editPassword->clear();
    m_ui->comboKeyFile->clear();
    m_ui->checkPassword->setChecked(false);
    m_ui->checkKeyFile->setChecked(false);
}
